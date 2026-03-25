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
#include "ECS/Component.h"
#include "ECS/Systems.h"
#include "Renderer/RendererDevice.h"
#include "Renderer/Mesh.h"

namespace Sunta
{
Scene::Scene() 
	: camera(glm::vec3(1.2f, 5.6f, 11.35f))
{
	
}
	
Scene::~Scene()
{
	EventBus::Unsubsribe(resizeEventID);
}
	
void Scene::Init(RendererDevice& rendererDevice, float windowWidth, float windowHeight)
{
	OnWindowResize(windowWidth, windowHeight);

	resizeEventID = EventBus::Subscribe<WindowResizeEvent>(
		[this](auto& event) { OnWindowResize(static_cast<float>(event.width), static_cast<float>(event.height)); }
	);
	
	ResourceManager::LoadTexture("container2Diffuse",	"res/Textures/container2.png");
	ResourceManager::LoadTexture("container2Specular",	"res/Textures/container2_specular.png");
	ResourceManager::LoadTexture("whiteTexture",		"res/Textures/whitePixel.png");
	ResourceManager::LoadTexture("errorTexture",		"res/Textures/errorTexture.png");

	ResourceManager::LoadTexture("cube_container",		"res/Textures/container.jpg");
	ResourceManager::LoadTexture("cube_chad",			"res/Textures/chad.png");

	ResourceManager::LoadShader ("reflectable",			"res/shaders/Reflectable.shader");
	ResourceManager::LoadShader ("lightSource",			"res/shaders/LightSource.shader");
	
	auto cubeDiffuseMap		= ResourceManager::GetTextureData("container2Diffuse");
	auto cubeSpecularMap	= ResourceManager::GetTextureData("container2Specular");

	auto texturedShader		= ResourceManager::GetShaderData("reflectable");
	auto cubeShader			= ResourceManager::GetShaderData("reflectable");
	auto lightShader		= ResourceManager::GetShaderData("lightSource");

	auto texturedMaterial	= std::make_shared<Material>(texturedShader, cubeDiffuseMap, cubeSpecularMap);
	auto cubeMaterial		= std::make_shared<Material>(cubeShader);
	auto lightMaterial		= std::make_shared<Material>(lightShader);

	cubeMaterial->SetAmbient(glm::vec3(0.25f, 0.2f, 0.05f))
		.SetDiffuse(glm::vec3(0.75f, 0.6f, 0.24f))
		.SetSpecular(glm::vec3(0.63, 0.56f, 0.37f))
		.SetShininess(128.0f);

	unsigned int cube = entityManager.CreateEntity();
	entityManager.AddComponent<TransformComponent>(cube, glm::vec3(7.5f, 5.0f, 3.0f));
	entityManager.AddComponent<WorldMatrixComponent>(cube);
	entityManager.AddComponent<MeshComponent>(cube, Primitives::CreateCube(rendererDevice), cubeMaterial);

	unsigned int texturedCube = entityManager.CreateEntity();
	entityManager.AddComponent<TransformComponent>(texturedCube, glm::vec3(0.0f, 5.0f, -0.5));
	entityManager.AddComponent<WorldMatrixComponent>(texturedCube);
	entityManager.AddComponent<MeshComponent>(texturedCube, Primitives::CreateCube(rendererDevice), texturedMaterial);

	unsigned int light = entityManager.CreateEntity();
	entityManager.AddComponent<TransformComponent>(light, glm::vec3(3.0f, 6.0f, 2.0f));
	entityManager.AddComponent<WorldMatrixComponent>(light);
	entityManager.AddComponent<MeshComponent>(light, Primitives::CreateCube(rendererDevice), lightMaterial);
	entityManager.AddComponent<LightComponent>(light, glm::vec3(0.2f), glm::vec3(0.5f), glm::vec3(1.0f));

	//AddEntity(std::move(cubeEntity));
	//AddEntity(std::move(lightSource));
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
}
	
void Scene::Render(Renderer& renderer)
{
	renderer.Clear(0.05f, 0.15f, 0.25f, 1.0f);
	SceneData sceneData = camera.GetSceneData();

	unsigned int totalEntites = entityManager.GetEntityCount();

	for (unsigned int i = 0; i < totalEntites; i++)
	{
		auto* light = entityManager.GetComponent<LightComponent>(i);
		auto* lightTransform = entityManager.GetComponent<TransformComponent>(i);

		if (light && lightTransform)
		{
			sceneData.lightSourceData.position          = lightTransform->position;
			sceneData.lightSourceData.ambientIntensity  = light->ambientIntensity;
			sceneData.lightSourceData.diffuseIntensity  = light->diffuseIntensity;
			sceneData.lightSourceData.specularIntensity = light->specularIntensity;
			break;
		}
	}

	

	/*auto& meshes = entityManager.GetAllComponents<MeshComponent>();
	auto& matricies = entityManager.GetAllComponents<WorldMatrixComponent>();

	for (int i = 0; i < meshes.size(); i++)
	{
		if (meshes[i].mesh && meshes[i].material)
			renderer.DrawMesh(*meshes[i].mesh, *meshes[i].material, matricies[i].matrix, sceneData);
	}*/

	//2ND JIRA COMMIT TEST
	//INITIAL CUBE LIGHT MAP TEST - JIRA
	for (unsigned int i = 0; i < totalEntites; i++)
	{
		auto* meshComponent = entityManager.GetComponent<MeshComponent>(i);
		auto* matrixComponent = entityManager.GetComponent<WorldMatrixComponent>(i);

		if (!(meshComponent && matrixComponent && meshComponent->mesh && meshComponent->material))
			continue;

		if (entityManager.GetComponent<LightComponent>(i))
			renderer.DrawLigthSource(*meshComponent->mesh, *meshComponent->material->GetShader(), matrixComponent->matrix, sceneData);
		else
			renderer.DrawMesh(*meshComponent->mesh, *meshComponent->material, matrixComponent->matrix, sceneData);
	}
}
	
void Scene::Clear()
{
	
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
