#include "Core/SuntaPreCompiled.h"

#include <glad/glad.h>

#include <glm/glm.hpp>

#include "Renderer/VertexBuffer.h"
#include "Renderer/ElementBuffer.h"

#include "Scene.h"
#include "Core/EngineTime.h"
#include "Core/InputManager.h"
#include "Scene/Camera.h"
#include "Core/ResourceManager.h"
#include "Renderer/Primitives.h"
#include "Renderer/Shader.h"
#include "Renderer/Renderer.h"
#include "Renderer/Material.h"
#include "Core/Log.h"
#include "Events/EventBus.h"
#include "Core/KeyCodes.h"
#include "Core/EngineAssets.h"
#include "ECS/Component.h"
#include "ECS/Systems.h"
#include "Renderer/RendererDevice.h"
#include "Renderer/Mesh.h"
#include "Utilities/MathUtilities.h"
#include "EntityFactory.h"

namespace Sunta
{
Scene::Scene() 
	: camera(glm::vec3(1.2f, 5.6f, 11.35f))
{
	
}
	
Scene::~Scene()
{
	EventBus::Unsubscribe(resizeEventID);
}
	
void Scene::Init(RendererDevice& rendererDevice, float windowWidth, float windowHeight)
{
	OnWindowResize(windowWidth, windowHeight);

	resizeEventID = EventBus::Subscribe<WindowResizeEvent>(
		[this](auto& event) { OnWindowResize(static_cast<float>(event.width), static_cast<float>(event.height)); }
	);

	using namespace Sunta::EngineAssets;

	auto texturedMaterial = ResourceManager::GetMaterialData(Materials::Textured);

	EntityFactory::CreateCube(*this, glm::vec3(7.5f, 5.0f, 3.0f), "Cube");
	EntityFactory::CreateCube(*this, glm::vec3(0.0f, 5.0f, -0.5), "Textured Cube", texturedMaterial);
	EntityFactory::CreateDirectionalLight(*this, glm::vec3(3.0f, 6.0f, 2.0f), "Directional Light");
	EntityFactory::CreatePointLight(*this, glm::vec3(-4.0f, 2.0f, 0.0f), "Point Light");
	EntityFactory::CreateSpotLight(*this, glm::vec3(-2.5f, 4.5f, 0.0f), "Spot Light");
}
	
void Scene::OnWindowResize(float windowWidth, float windowHeight)
{
	camera.SetViewportSize(windowWidth, windowHeight);
}
	
void Scene::ProcessInput()
{
	float deltaTime				= EngineTime::deltaTime;
	glm::vec2 mouseDelta		= InputManager::GetMouseDelta();
	
	camera.HandleMouseMovement(mouseDelta.x, mouseDelta.y);
	float scroll = InputManager::GetScrollOffset();
	if (scroll != 0.0f)
		camera.HandleScrolling(scroll);
	
	camera.HandleStayOnHeight(InputManager::IsKeyPressed(SUNTA_KEY_LEFT_ALT));
	
	if (InputManager::IsKeyPressed(SUNTA_KEY_W))
		camera.HandleKeyboardMove(MOVEMENT::FORWARD	, deltaTime);
	if (InputManager::IsKeyPressed(SUNTA_KEY_S))
		camera.HandleKeyboardMove(MOVEMENT::BACKWARD, deltaTime);
	if (InputManager::IsKeyPressed(SUNTA_KEY_A))
		camera.HandleKeyboardMove(MOVEMENT::LEFT	, deltaTime);
	if (InputManager::IsKeyPressed(SUNTA_KEY_D))
		camera.HandleKeyboardMove(MOVEMENT::RIGHT	, deltaTime);
	
	if (InputManager::IsKeyPressed(SUNTA_KEY_SPACE))
		camera.HandleKeyboardMove(MOVEMENT::UP		, deltaTime);
	if (InputManager::IsKeyPressed(SUNTA_KEY_LEFT_SHIFT))
		camera.HandleKeyboardMove(MOVEMENT::DOWN	, deltaTime);
	
}
	
void Scene::Update()
{
	Systems::UpdateTransform(entityManager);
	Systems::SyncMeshComponents(entityManager);
}
	
void Scene::Render(Renderer& renderer)
{
	SceneData sceneData = camera.GetSceneData();

	sceneData.directionalLights.clear();
	sceneData.pointLights.clear();
	sceneData.spotlights.clear();

	unsigned int totalEntites = entityManager.GetEntityCount();

	for (unsigned int i = 0; i < totalEntites; i++)
	{
		auto* lightTransform = entityManager.GetComponent<TransformComponent>(i);

		if (auto* directionalLightComponent = entityManager.GetComponent<DirectionalLightComponent>(i))
		{
			if (!lightTransform)
				continue;

			DirectionalLightData data;

			data.direction = Sunta::Math::DegreesToDirection(lightTransform->rotation);
			data.color	   = directionalLightComponent->color;

			sceneData.directionalLights.push_back(data);
		}

		if (auto* pointlightComponent = entityManager.GetComponent<PointLightComponent>(i))
		{
			if (!lightTransform)
				continue;

			PointLightData data;

			data.position	 = lightTransform->position;

			data.color		 = pointlightComponent->color;
			data.attenuation = pointlightComponent->attenuation;

			sceneData.pointLights.push_back(data);
		}

		if (auto* spotlightComponent = entityManager.GetComponent<SpotLightComponent>(i))
		{
			if (!lightTransform)
				continue;

			SpotlightData data;

			data.position		     = lightTransform->position;
			data.spotlightDirection	 = Sunta::Math::DegreesToDirection(lightTransform->rotation);

			data.innercutOffAngle	 = spotlightComponent->innerCutOffAngle;
			data.outerCutOffAngle	 = spotlightComponent->outerCutOffAngle;

			data.color               = spotlightComponent->color;
			data.attenuation         = spotlightComponent->attenuation;
			
			sceneData.spotlights.push_back(data);
		}
	}

	struct RenderItem 
	{ 
		Mesh* mesh; 
		Material* material; 
		glm::mat4 matrix; 
	};

	std::vector<RenderItem> litQueue;
	std::vector<RenderItem> unlitQueue;
	std::vector<RenderItem> lightSourceQueue;

	for (unsigned int i = 0; i < totalEntites; i++)
	{
		auto* meshComponent = entityManager.GetComponent<MeshComponent>(i);
		auto* matrixComponent = entityManager.GetComponent<WorldMatrixComponent>(i);

		if (!meshComponent || !matrixComponent || !meshComponent->mesh || !meshComponent->material)
			continue;

		if (!meshComponent->isVisible)
			continue;

		bool isLightSource = entityManager.GetComponent<DirectionalLightComponent>(i) ||
							 entityManager.GetComponent<PointLightComponent>(i)       ||
							 entityManager.GetComponent<SpotLightComponent>(i);

		RenderItem item = { meshComponent->mesh.get(), meshComponent->material.get(), matrixComponent->matrix };

		if (isLightSource)
		{
			lightSourceQueue.push_back(item);
		}
		else
		{
			if (item.material->GetShader()->HasFeature(ShaderFeature::Lighting))
				litQueue.push_back(item);
			else
				unlitQueue.push_back(item);
		}

	}

	for (const auto& item : lightSourceQueue)
		renderer.DrawLigthSource(*item.mesh, *item.material->GetShader(), item.matrix, sceneData);

	for (const auto& item : litQueue)
		renderer.DrawMesh(*item.mesh, *item.material, item.matrix, sceneData);

	for (const auto& item : unlitQueue)
		renderer.DrawMesh(*item.mesh, *item.material, item.matrix, sceneData);

}
	
void Scene::Clear()
{
	entityManager.Clear();
}

//void Scene::AddEntity(std::unique_ptr<Entity> entity)
//{
//	
//	if (auto* light = dynamic_cast<LightSource*>(entity.get()))
//		sceneLights.push_back(light);
//
//	sceneEntities.push_back(std::move(entity));
//
//}

//std::vector<Inspectable*> Scene::GetInspectables()
//{
//	std::vector<Inspectable*> inspectables;
//		
//	for (auto& entity : sceneEntities)
//		inspectables.push_back(entity.get());
//
//	return inspectables;
//}

}
