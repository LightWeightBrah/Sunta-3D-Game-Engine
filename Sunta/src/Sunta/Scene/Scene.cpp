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
#include "Gameplay/CharacterAnimationSystem.h"
#include "Gameplay/CharacterControllerSystem.h"
#include "Gameplay/FollowCameraSystem.h"
#include "Gameplay/LockOnSystem.h"

namespace Sunta
{

namespace
{
	// The camera of an entity looks along the -Z axis of the entity, with +Y up
	const glm::vec3 CameraLocalForward = glm::vec3(0.0f, 0.0f, -1.0f);
	const glm::vec3 CameraLocalUp      = glm::vec3(0.0f, 1.0f,  0.0f);

	// The frustum drawn for a camera in the editor is short, the real far plane would be far outside the screen
	constexpr float CameraGizmoDepth            = 2.0f;
	constexpr float CameraGizmoMaxFieldOfView   = 120.0f;
	constexpr unsigned int CameraGizmoRingCorners = 4;
	const glm::vec3 CameraGizmoColor = glm::vec3(1.0f, 0.85f, 0.2f);

	// The line of sight of the lock-on (View > Show Lock-On Sight Rays)
	const glm::vec3 SightRayClearColor      = glm::vec3(0.2f, 1.0f, 0.2f);
	const glm::vec3 SightRayBlockedColor    = glm::vec3(1.0f, 0.15f, 0.15f);
	const glm::vec3 SightRayHiddenPartColor = glm::vec3(0.5f, 0.5f, 0.5f);
	const glm::vec3 SightRayHitColor        = glm::vec3(1.0f, 0.6f, 0.0f);
	constexpr float SightRayHitMarkHalfSize = 0.25f;

	// A camera has no mesh, so the mouse picks it by a small box in front of it
	const glm::vec3 CameraPickingBoxMin = glm::vec3(-0.3f, -0.2f, -0.8f);
	const glm::vec3 CameraPickingBoxMax = glm::vec3( 0.3f,  0.2f,  0.0f);

	void DrawSightRay(const LockOnSystem::SightRay& sightRay, const SceneData& sceneData)
	{
		if (!sightRay.isBlocked)
		{
			DebugRenderer::DrawLine(sightRay.from, sightRay.to, SightRayClearColor, sceneData);
			return;
		}

		DebugRenderer::DrawLine(sightRay.from, sightRay.hitPoint, SightRayBlockedColor, sceneData);
		DebugRenderer::DrawLine(sightRay.hitPoint, sightRay.to, SightRayHiddenPartColor, sceneData);
		DebugRenderer::DrawCross(sightRay.hitPoint, SightRayHitMarkHalfSize, SightRayHitColor, sceneData);
	}

	float GetClampedFieldOfView(const CameraComponent& camera)
	{
		return glm::clamp(camera.fieldOfView, CameraComponent::MinFieldOfView, CameraComponent::MaxFieldOfView);
	}

	glm::mat4 MakeCameraViewMatrix(const TransformComponent& transform)
	{
		const glm::quat rotation = glm::normalize(transform.rotationQuaternion);

		const glm::vec3 forward = rotation * CameraLocalForward;
		const glm::vec3 up      = rotation * CameraLocalUp;

		return glm::lookAt(transform.position, transform.position + forward, up);
	}

	glm::mat4 MakeCameraProjectionMatrix(const CameraComponent& camera, float aspectRatio)
	{
		const float nearPlane = glm::max(camera.nearPlane, CameraComponent::MinNearPlane);
		const float farPlane  = glm::max(camera.farPlane, nearPlane + CameraComponent::MinNearPlane);

		return glm::perspective(glm::radians(GetClampedFieldOfView(camera)), aspectRatio, nearPlane, farPlane);
	}

	// The direction a view looks in. In a view matrix the third row points BACKWARDS, so forward is its opposite
	glm::vec3 GetViewForward(const glm::mat4& viewMatrix)
	{
		return -glm::vec3(viewMatrix[0][2], viewMatrix[1][2], viewMatrix[2][2]);
	}

	// Corners of the frustum in the order the box wireframe expects them:
	// the near rectangle (4 corners going around), then the far rectangle (the same order)
	std::array<glm::vec3, BOX_CORNER_COUNT> MakeCameraFrustumCorners(const TransformComponent& transform, const CameraComponent& camera, float aspectRatio)
	{
		const glm::quat rotation = glm::normalize(transform.rotationQuaternion);

		const float fieldOfView = glm::min(GetClampedFieldOfView(camera), CameraGizmoMaxFieldOfView);
		const float tangent     = std::tan(glm::radians(fieldOfView) * 0.5f);

		const float nearDepth = glm::min(glm::max(camera.nearPlane, CameraComponent::MinNearPlane), CameraGizmoDepth);
		const float ringDepths[2] = { nearDepth, CameraGizmoDepth };

		std::array<glm::vec3, BOX_CORNER_COUNT> corners{};

		for (unsigned int ring = 0; ring < 2; ring++)
		{
			const float depth      = ringDepths[ring];
			const float halfHeight = depth * tangent;
			const float halfWidth  = halfHeight * aspectRatio;

			const glm::vec3 ringCorners[CameraGizmoRingCorners] =
			{
				{ -halfWidth, -halfHeight, -depth },
				{ +halfWidth, -halfHeight, -depth },
				{ +halfWidth, +halfHeight, -depth },
				{ -halfWidth, +halfHeight, -depth }
			};

			for (unsigned int corner = 0; corner < CameraGizmoRingCorners; corner++)
				corners[ring * CameraGizmoRingCorners + corner] = transform.position + rotation * ringCorners[corner];
		}

		return corners;
	}
}

Scene::Scene() 
	: camera(glm::vec3(1.2f, 5.6f, 11.35f))
{
	
}
	
Scene::~Scene()
{
	EventBus::Unsubscribe(resizeEventID);
	EventBus::Unsubscribe(engineModeEventID);

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

	engineModeEventID = EventBus::Subscribe<EngineModeChangedEvent>(
		[this](const EngineModeChangedEvent& event) { OnEngineModeChanged(event); }
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

	UpdateActiveCameraData();
}
	
void Scene::OnWindowResize(float windowWidth, float windowHeight)
{
	camera.SetViewportSize(windowWidth, windowHeight);

	// A minimized window has no size, the last proper aspect ratio stays
	if (windowWidth > 0.0f && windowHeight > 0.0f)
		aspectRatio = windowWidth / windowHeight;
}
	
void Scene::OnEngineModeChanged(const EngineModeChangedEvent& event)
{
	isMouseOwnedByGame = (event.mode == EngineMode::Game);
}

glm::vec2 Scene::GetGameLookDelta() const
{
	return isMouseOwnedByGame ? InputManager::GetMouseDelta() : glm::vec2(0.0f);
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
	// The camera of the game is moved by the game (scripts),
	// so the free camera must not fly away on the keys at the same time
	if (ShouldUseSceneCamera())
		return;

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

	const glm::vec3 cameraForward = GetViewForward(activeCameraData.viewMatrix);
	const glm::vec2 lookDelta     = isPlaying ? GetGameLookDelta() : glm::vec2(0.0f);

	if (isPlaying)
	{
		Systems::UpdateScripts(entityManager, EngineTime::deltaTime, isScriptHotReloadEnabled);
		LockOnSystem::SetRecordingSightRays(DebugRenderer::GetShowLockOnSightRays());
		LockOnSystem::Update(entityManager, EngineTime::deltaTime, cameraForward, lookDelta);
		CharacterControllerSystem::Update(entityManager, EngineTime::deltaTime, cameraForward);
		PhysicsSystem::UpdatePhysics(entityManager, EngineTime::deltaTime);
	}

	Systems::SyncMeshComponents(entityManager);
	Systems::SyncModelComponents(entityManager);

	if (isPlaying)
	{
		// After physics (needs the speed of the character), before the animators play the chosen animation
		CharacterAnimationSystem::Update(entityManager);
		Systems::UpdateAnimators(entityManager, EngineTime::deltaTime);
	}

	Systems::UpdateTransform(entityManager);

	// After the transforms, so the camera sees where the player and the walls really are
	if (isPlaying)
		FollowCameraSystem::Update(entityManager, EngineTime::deltaTime, lookDelta);

	// Keeps collider gizmos following their entities while stopped
	if (!isPlaying)
		CollisionSystem::UpdateColliders(entityManager);

	// After the transforms, so a camera that follows a moving entity is never a frame behind
	UpdateActiveCameraData();
}
	
void Scene::Render(Renderer& renderer)
{
	const bool      isPlaying        = IsPlaying();
	const bool      showBoundsGizmos = DebugRenderer::GetShowBoundsGizmos();
	const glm::vec3 boundsGizmoColor = DebugRenderer::GetBoundsGizmoColor();

	SceneData sceneData = activeCameraData;

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

		// A camera is invisible in the game, in the editor its frustum shows where it stands and what it sees
		if (!isPlaying && transformComponent)
		{
			if (auto* cameraComponent = entityManager.GetComponent<CameraComponent>(i))
				gizmosQueue.push_back({ MakeCameraFrustumCorners(*transformComponent, *cameraComponent, aspectRatio), CameraGizmoColor });
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

	if (isPlaying)
	{
		for (const LockOnSystem::SightRay& sightRay : LockOnSystem::GetSightRays())
			DrawSightRay(sightRay, sceneData);
	}

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
	else if (entityManager.GetComponent<CameraComponent>(entity))
	{
		localBoundsMin = CameraPickingBoxMin;
		localBoundsMax = CameraPickingBoxMax;
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
	Ray ray = Camera::ScreenPointToRay(mousePosition, viewportWidth, viewportHeight, GetViewMatrix(), GetProjectionMatrix());

	unsigned int hitEntityID = 0;
	float        hitDistance = 0.0f;

	if (TryFindClosestEntityHitByRay(ray, hitEntityID, hitDistance))
		return static_cast<int>(hitEntityID);

	return -1;
}

glm::vec3 Scene::GetWorldPositionUnderMouse(const glm::vec2& mousePosition, float viewportWidth, float viewportHeight)
{
	Ray ray = Camera::ScreenPointToRay(mousePosition, viewportWidth, viewportHeight, GetViewMatrix(), GetProjectionMatrix());

	// The mouse is over an entity: use the point on its surface
	unsigned int hitEntityID = 0;
	float        hitDistance = 0.0f;
	if (TryFindClosestEntityHitByRay(ray, hitEntityID, hitDistance))
		return ray.origin + ray.direction * hitDistance;

	// The mouse is over empty space: use a point at a fixed distance from the camera, along the ray
	constexpr float DISTANCE_WHEN_NOTHING_IS_HIT = 10.0f;
	return ray.origin + ray.direction * DISTANCE_WHEN_NOTHING_IS_HIT;
}

void Scene::BeginPlay()
{
	CollisionSystem::Reset();
	LockOnSystem::ClearSightRays();
	CharacterControllerSystem::ReportProblemsBeforePlay(entityManager);
	FollowCameraSystem::ReportProblemsBeforePlay(entityManager);
	CharacterAnimationSystem::ReportProblemsBeforePlay(entityManager);
	playState = PlayState::Playing;

	CameraComponent* cameraComponent = nullptr;
	TransformComponent* cameraTransform = nullptr;
	if (!TryFindActiveSceneCamera(cameraComponent, cameraTransform))
		SUNTA_ENGINE_LOG_WARNING("Scene: There is no active Camera component, the game uses the free camera. Add a Camera component to an entity");

	// The first frame of the game is already drawn from the camera of the scene
	UpdateActiveCameraData();
}

bool Scene::TryFindActiveSceneCamera(CameraComponent*& outCamera, TransformComponent*& outTransform)
{
	for (CameraComponent& cameraComponent : entityManager.GetAllComponents<CameraComponent>())
	{
		if (!cameraComponent.isActive)
			continue;

		const unsigned int entity = entityManager.GetEntityIDForComponent(cameraComponent);

		TransformComponent* transform = entityManager.GetComponent<TransformComponent>(entity);
		if (!transform)
			continue;

		outCamera    = &cameraComponent;
		outTransform = transform;
		return true;
	}

	return false;
}

bool Scene::ShouldUseSceneCamera()
{
	if (!IsPlaying())
		return false;

	CameraComponent*    cameraComponent = nullptr;
	TransformComponent* cameraTransform = nullptr;

	return TryFindActiveSceneCamera(cameraComponent, cameraTransform);
}

void Scene::UpdateActiveCameraData()
{
	CameraComponent*    cameraComponent = nullptr;
	TransformComponent* cameraTransform = nullptr;

	if (IsPlaying() && TryFindActiveSceneCamera(cameraComponent, cameraTransform))
	{
		activeCameraData.viewMatrix       = MakeCameraViewMatrix(*cameraTransform);
		activeCameraData.projectionMatrix = MakeCameraProjectionMatrix(*cameraComponent, aspectRatio);
		activeCameraData.cameraPosition   = cameraTransform->position;
		return;
	}

	const SceneData freeCameraData = camera.GetSceneData();

	activeCameraData.viewMatrix       = freeCameraData.viewMatrix;
	activeCameraData.projectionMatrix = freeCameraData.projectionMatrix;
	activeCameraData.cameraPosition   = freeCameraData.cameraPosition;
}

void Scene::AlignEntityWithEditorCamera(unsigned int entityID)
{
	TransformComponent* transform = entityManager.GetComponent<TransformComponent>(entityID);
	if (!transform)
		return;

	transform->position           = camera.GetPosition();
	transform->rotationQuaternion = camera.GetRotation();
	transform->SyncEulerFromQuaternion();
	transform->isDirty            = true;
}

void Scene::Clear()
{
	entityManager.Clear();
}

}
