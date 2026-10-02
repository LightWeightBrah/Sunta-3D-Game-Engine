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
#include "Renderer/DebugRenderer.h"
#include "Physics/CollisionSystem.h"
#include "Physics/CollisionShapes.h"
#include "Physics/PhysicsSystem.h"

namespace Sunta
{
Scene::Scene() 
	: camera(glm::vec3(1.2f, 5.6f, 11.35f))
{
	
}
	
Scene::~Scene()
{
	EventBus::Unsubscribe(resizeEventID);

	EventBus::Unsubscribe(triggerEnterEventID);
	EventBus::Unsubscribe(triggerStayEventID);
	EventBus::Unsubscribe(triggerExitEventID);

	EventBus::Unsubscribe(collisionEnterEventID);
	EventBus::Unsubscribe(collisionStayEventID);
	EventBus::Unsubscribe(collisionExitEventID);
}
	
void Scene::Init(RendererDevice& rendererDevice, float windowWidth, float windowHeight)
{
	OnWindowResize(windowWidth, windowHeight);

	resizeEventID = EventBus::Subscribe<WindowResizeEvent>(
		[this](auto& event) { OnWindowResize(static_cast<float>(event.width), static_cast<float>(event.height)); }
	);

	// Trigger Events

	triggerEnterEventID = EventBus::Subscribe<TriggerEnterEvent>(
		[this](const TriggerEnterEvent& event) { OnTriggerEnter(event); }
	);
	triggerStayEventID = EventBus::Subscribe<TriggerStayEvent>(
		[this](const TriggerStayEvent& event) { OnTriggerStay(event); }
	);
	triggerExitEventID = EventBus::Subscribe<TriggerExitEvent>(
		[this](const TriggerExitEvent& event) { OnTriggerExit(event); }
	);

	// Collision Events

	collisionEnterEventID = EventBus::Subscribe<CollisionEnterEvent>(
		[this](const CollisionEnterEvent& event) { OnCollisionEnter(event); }
	);
	collisionStayEventID = EventBus::Subscribe<CollisionStayEvent>(
		[this](const CollisionStayEvent& event) { OnCollisionStay(event); }
	);
	collisionExitEventID = EventBus::Subscribe<CollisionExitEvent>(
		[this](const CollisionExitEvent& event) { OnCollisionExit(event); }
	);
}
	
void Scene::OnWindowResize(float windowWidth, float windowHeight)
{
	camera.SetViewportSize(windowWidth, windowHeight);
}
	
// Trigger Events

void Scene::OnTriggerEnter(const TriggerEnterEvent& event)
{
	Systems::DispatchTriggerEnter(entityManager, event.triggerEntityID, event.otherEntityID);
}

void Scene::OnTriggerStay(const TriggerStayEvent& event)
{
	Systems::DispatchTriggerStay(entityManager, event.triggerEntityID, event.otherEntityID);
}

void Scene::OnTriggerExit(const TriggerExitEvent& event)
{
	Systems::DispatchTriggerExit(entityManager, event.triggerEntityID, event.otherEntityID);
}

// Collision Events

void Scene::OnCollisionEnter(const CollisionEnterEvent& event)
{
	Systems::DispatchCollisionEnter(entityManager, event.collisionEntityID, event.otherEntityID);
}

void Scene::OnCollisionStay(const CollisionStayEvent& event)
{
	Systems::DispatchCollisionStay(entityManager, event.collisionEntityID, event.otherEntityID);
}

void Scene::OnCollisionExit(const CollisionExitEvent& event)
{
	Systems::DispatchCollisionExit(entityManager, event.collisionEntityID, event.otherEntityID);
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
	const bool isPlaying = IsPlaying();

	if (isPlaying)
		PhysicsSystem::UpdatePhysics(entityManager, EngineTime::deltaTime);

	Systems::SyncMeshComponents(entityManager);
	Systems::SyncModelComponents(entityManager);

	if (isPlaying)
	{
		Systems::UpdateAnimators(entityManager, EngineTime::deltaTime);
		Systems::UpdateScripts(entityManager, EngineTime::deltaTime);
	}

	Systems::UpdateTransform(entityManager);

	// Keeps collider gizmos following their entities while stopped
	if (!isPlaying)
		CollisionSystem::UpdateColliders(entityManager);
}
	
void Scene::Render(Renderer& renderer)
{
	const bool      showBoundsGizmos = DebugRenderer::GetShowBoundsGizmos();
	const glm::vec3 boundsGizmoColor = DebugRenderer::GetBoundsGizmoColor();

	SceneData sceneData = camera.GetSceneData();

	sceneData.directionalLights.clear();
	sceneData.pointLights.clear();
	sceneData.spotlights.clear();

	unsigned int totalEntites = entityManager.GetEntityCount();

	struct MeshRenderItem
	{
		Mesh* mesh;
		Material* material;

		glm::mat4 matrix;
	};

	struct ModelRenderItem
	{
		Model* model;
		const Animator* animator;

		glm::mat4 matrix;
	};

	struct GizmosDrawItem
	{
		std::array<glm::vec3, BOX_CORNER_COUNT> corners;
		glm::vec3 color;
	};

	std::vector<MeshRenderItem>  litQueue;
	std::vector<MeshRenderItem>  unlitQueue;
	std::vector<MeshRenderItem>  lightSourceQueue;
	std::vector<ModelRenderItem> modelQueue;

	std::vector<GizmosDrawItem>  gizmosQueue;

	for (unsigned int i = 0; i < totalEntites; i++)
	{
		auto* transformComponent        = entityManager.GetComponent<TransformComponent>(i);
		auto* matrixComponent           = entityManager.GetComponent<WorldMatrixComponent>(i);

		auto* directionalLightComponent = entityManager.GetComponent<DirectionalLightComponent>(i);
		auto* pointLightComponent       = entityManager.GetComponent<PointLightComponent>(i);
		auto* spotLightComponent        = entityManager.GetComponent<SpotLightComponent>(i);

		if (directionalLightComponent && transformComponent)
		{
			DirectionalLightData data;

			data.direction   = Sunta::Math::DegreesToDirection(transformComponent->rotation);
			data.color       = directionalLightComponent->color;

			sceneData.directionalLights.push_back(data);
		}

		if (pointLightComponent && transformComponent)
		{
			PointLightData data;

			data.position	 = transformComponent->position;

			data.color		 = pointLightComponent->color;
			data.attenuation = pointLightComponent->attenuation;

			sceneData.pointLights.push_back(data);
		}

		if (spotLightComponent && transformComponent)
		{
			SpotlightData data;

			data.position		     = transformComponent->position;
			data.spotlightDirection	 = Sunta::Math::DegreesToDirection(transformComponent->rotation);

			data.innercutOffAngle	 = spotLightComponent->innerCutOffAngle;
			data.outerCutOffAngle	 = spotLightComponent->outerCutOffAngle;

			data.color               = spotLightComponent->color;
			data.attenuation         = spotLightComponent->attenuation;
			
			sceneData.spotlights.push_back(data);
		}

		if (auto* meshComponent = entityManager.GetComponent<MeshComponent>(i))
		{
			if (matrixComponent && meshComponent->isVisible && meshComponent->mesh && meshComponent->material)
			{
				bool isLightSource = directionalLightComponent || pointLightComponent || spotLightComponent;

				MeshRenderItem item = { meshComponent->mesh.get(), meshComponent->material.get(), matrixComponent->matrix };

				if (isLightSource)
					lightSourceQueue.push_back(item);
				else if(item.material->GetShader()->HasFeature(ShaderFeature::Lighting))
					litQueue.push_back(item);
				else
					unlitQueue.push_back(item);
			}
		}

		if (auto* modelComponent = entityManager.GetComponent<ModelComponent>(i))
		{
			if (matrixComponent && modelComponent->isVisible && modelComponent->modelData && modelComponent->modelData->model)
			{
				auto* animatorComponent = entityManager.GetComponent<AnimatorComponent>(i);
				const Animator* animator = animatorComponent ? &animatorComponent->animator : nullptr;

				modelQueue.push_back({ modelComponent->modelData->model.get(), animator, matrixComponent->matrix });
			}
		}

		// --- DEBUG: Bounding box used for mouse picking ---
		if (showBoundsGizmos)
		{
			OBB pickingBox;
			if (TryGetEntityWorldPickingBox(i, pickingBox))
				gizmosQueue.push_back({ GetOBBCorners(pickingBox), boundsGizmoColor });
		}

		if (auto* boxCollider = entityManager.GetComponent<BoxColliderComponent>(i))
		{
			bool shouldDrawThisGizmos = DebugRenderer::GetShowAllGizmos() || boxCollider->showGizmos;

			if (shouldDrawThisGizmos)
			{
				bool isOverlapping = CollisionSystem::IsEntityOverlapping(i);
				glm::vec3 color = isOverlapping ?
					DebugRenderer::GetGizmosCollideColor() : DebugRenderer::GetGizmosColor();

				gizmosQueue.push_back({ GetOBBCorners(boxCollider->worldOBB), color });
			}
		}

	}

	for (const auto& item : lightSourceQueue)
		renderer.DrawLigthSource(*item.mesh, *item.material->GetShader(), item.matrix, sceneData);

	for (const auto& item : litQueue)
		renderer.DrawMesh(*item.mesh, *item.material, item.matrix, sceneData);

	for (const auto& item : unlitQueue)
		renderer.DrawMesh(*item.mesh, *item.material, item.matrix, sceneData);

	for (const auto& item : modelQueue)
		renderer.DrawModel(*item.model, item.matrix, sceneData, item.animator);

	for (const auto& item : gizmosQueue)
		DebugRenderer::DrawBoxWireframe(item.corners, item.color, sceneData);

}

bool Scene::TryGetEntityWorldPickingBox(unsigned int entity, OBB& outBox)
{
	auto* worldMatrixComponent = entityManager.GetComponent<WorldMatrixComponent>(entity);
	if (!worldMatrixComponent)
		return false;

	glm::vec3 localBoundsMin;
	glm::vec3 localBoundsMax;

	if (auto* meshComponent = entityManager.GetComponent<MeshComponent>(entity))
	{
		if (!meshComponent->isVisible || !meshComponent->mesh)
			return false;

		localBoundsMin = meshComponent->mesh->GetLocalBoundsMin();
		localBoundsMax = meshComponent->mesh->GetLocalBoundsMax();
	}
	else if (auto* modelComponent = entityManager.GetComponent<ModelComponent>(entity))
	{
		if (!modelComponent->isVisible || !modelComponent->modelData || !modelComponent->modelData->model)
			return false;

		localBoundsMin = modelComponent->modelData->model->GetLocalBoundsMin();
		localBoundsMax = modelComponent->modelData->model->GetLocalBoundsMax();
	}
	else
	{
		return false;
	}

	glm::vec3 localCenter      = (localBoundsMin + localBoundsMax) * 0.5f;
	glm::vec3 localHalfExtents = (localBoundsMax - localBoundsMin) * 0.5f;

	outBox = MakeWorldOBB(worldMatrixComponent->matrix, localCenter, localHalfExtents);
	return true;
}

bool Scene::TryFindClosestEntityHitByRay(const Ray& ray, unsigned int& outEntityID, float& outDistance)
{
	bool hitAnyEntity = false;
	outDistance = std::numeric_limits<float>::max();

	unsigned int totalEntities = entityManager.GetEntityCount();
	for (unsigned int entity = 0; entity < totalEntities; entity++)
	{
		OBB pickingBox;
		if (!TryGetEntityWorldPickingBox(entity, pickingBox))
			continue;

		float hitDistance = 0.0f;
		if (!RayIntersectsOBB(ray, pickingBox, hitDistance))
			continue;

		if (hitDistance < outDistance)
		{
			outDistance = hitDistance;
			outEntityID = entity;
			hitAnyEntity = true;
		}
	}

	return hitAnyEntity;
}

int Scene::GetEntityUnderMouse(const glm::vec2& mousePosition, float viewportWidth, float viewportHeight)
{
	Ray ray = camera.GetMouseScreenPositionToPointRay(mousePosition, viewportWidth, viewportHeight);

	unsigned int hitEntityID = 0;
	float        hitDistance = 0.0f;

	if (TryFindClosestEntityHitByRay(ray, hitEntityID, hitDistance))
		return static_cast<int>(hitEntityID);

	return -1;
}

glm::vec3 Scene::GetWorldPositionUnderMouse(const glm::vec2& mousePosition, float viewportWidth, float viewportHeight)
{
	Ray ray = camera.GetMouseScreenPositionToPointRay(mousePosition, viewportWidth, viewportHeight);

	// The mouse is over an entity: use the point on its surface
	unsigned int hitEntityID = 0;
	float        hitDistance = 0.0f;
	if (TryFindClosestEntityHitByRay(ray, hitEntityID, hitDistance))
		return ray.origin + ray.direction * hitDistance;

	// The mouse is over empty space: use a point at a fixed distance from the camera, along the ray
	constexpr float DISTANCE_WHEN_NOTHING_IS_HIT = 10.0f;
	return ray.origin + ray.direction * DISTANCE_WHEN_NOTHING_IS_HIT;
}

void Scene::Clear()
{
	entityManager.Clear();
}

}
