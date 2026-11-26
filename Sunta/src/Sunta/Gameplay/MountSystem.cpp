#include "Core/SuntaPreCompiled.h"
#include "MountSystem.h"

#include <glm/gtc/quaternion.hpp>

#include "CharacterActionsSystem.h"
#include "CharacterControllerSystem.h"
#include "EntityName.h"
#include "GameplayMath.h"
#include "HUDSystem.h"
#include "MountComponent.h"
#include "Core/InputManager.h"
#include "Core/KeyCodes.h"
#include "Core/Log.h"
#include "ECS/Component.h"
#include "ECS/EntityManager.h"

namespace Sunta
{

namespace
{

using namespace GameplayMath;

constexpr int MountKey = SUNTA_KEY_E;
constexpr int GallopKey = SUNTA_KEY_LEFT_SHIFT;
// The rider gets off this far to the mount's side
constexpr float DismountSideMeters = 1.3f;

float KeyAxis(int positiveKey, int negativeKey)
{
	return (InputManager::IsKeyPressed(positiveKey) ? 1.0f : 0.0f) - (InputManager::IsKeyPressed(negativeKey) ? 1.0f : 0.0f);
}

// W away from the camera, D to its right (as the player's own walking)
glm::vec3 ReadSteering(const glm::vec3& cameraForward)
{
	const glm::vec3 forward = GetHorizontalDirectionOrDefault(cameraForward);
	const glm::vec3 right   = glm::cross(forward, WorldUp);
	glm::vec3 direction = forward * KeyAxis(SUNTA_KEY_W, SUNTA_KEY_S) + right * KeyAxis(SUNTA_KEY_D, SUNTA_KEY_A);
	const float length = glm::length(direction);
	return length > 1.0f ? direction / length : direction;
}

FollowCameraComponent* FindFollowCamera(EntityManager& entityManager)
{
	for (FollowCameraComponent& camera : entityManager.GetAllComponents<FollowCameraComponent>())
		return &camera;
	return nullptr;
}

bool CanMount(EntityManager& entityManager, unsigned int riderID)
{
	const CharacterActionsComponent* actions = entityManager.GetComponent<CharacterActionsComponent>(riderID);
	const HealthComponent* health = entityManager.GetComponent<HealthComponent>(riderID);
	return !(actions && actions->IsBusy()) && !(health && health->isDead);
}

void Mount(EntityManager& entityManager, MountComponent& mount, unsigned int mountID, unsigned int riderID)
{
	PhysicsBodyComponent*         body       = entityManager.GetComponent<PhysicsBodyComponent>(riderID);
	BoxColliderComponent*         collider   = entityManager.GetComponent<BoxColliderComponent>(riderID);
	CharacterControllerComponent* controller = entityManager.GetComponent<CharacterControllerComponent>(riderID);
	CharacterAnimationComponent*  animation  = entityManager.GetComponent<CharacterAnimationComponent>(riderID);

	mount.isRidden = true;
	mount.riderID  = riderID;
	if (body)
	{
		mount.wasRiderKinematic = body->isKinematic;
		body->isKinematic = true;
		body->velocity    = glm::vec3(0.0f);
	}
	if (collider)
	{
		mount.wasRiderTrigger = collider->isTrigger;
		collider->isTrigger   = true;
	}
	if (controller)
	{
		controller->isRiding      = true;
		controller->moveDirection = glm::vec3(0.0f);
		controller->wantsToRun    = false;
	}
	if (animation)
	{
		mount.savedIdleAnimation         = animation->idleAnimation;
		mount.savedLockedOnIdleAnimation = animation->lockedOnIdleAnimation;
	}
	CharacterActionsSystem::Reset(entityManager, riderID);
	if (FollowCameraComponent* camera = FindFollowCamera(entityManager))
	{
		camera->pivotHeight += mount.cameraRise;
		camera->distance    += mount.cameraPullBack;
	}

	SUNTA_ENGINE_LOG_INFO("Mount: '{0}' rides '{1}'", GetEntityName(entityManager, riderID), GetEntityName(entityManager, mountID));
}

}

void MountSystem::Dismount(EntityManager& entityManager, unsigned int mountID)
{
	MountComponent* mount = entityManager.GetComponent<MountComponent>(mountID);
	if (!mount || !mount->isRidden)
		return;

	const unsigned int riderID = mount->riderID;
	PhysicsBodyComponent*         body       = entityManager.GetComponent<PhysicsBodyComponent>(riderID);
	BoxColliderComponent*         collider   = entityManager.GetComponent<BoxColliderComponent>(riderID);
	CharacterControllerComponent* controller = entityManager.GetComponent<CharacterControllerComponent>(riderID);
	CharacterAnimationComponent*  animation  = entityManager.GetComponent<CharacterAnimationComponent>(riderID);
	TransformComponent*           rider      = entityManager.GetComponent<TransformComponent>(riderID);
	const TransformComponent*     beast      = entityManager.GetComponent<TransformComponent>(mountID);

	if (rider && beast)
	{
		// Down on the left side, on the ground the mount stands on
		const glm::vec3 side = beast->rotationQuaternion * glm::vec3(-1.0f, 0.0f, 0.0f);
		rider->position   = beast->position + GetHorizontalDirectionOrDefault(side) * DismountSideMeters;
		rider->isDirty    = true;
	}
	if (body)
	{
		body->isKinematic = mount->wasRiderKinematic;
		body->velocity    = glm::vec3(0.0f);
	}
	if (collider)
		collider->isTrigger = mount->wasRiderTrigger;
	if (controller)
		controller->isRiding = false;
	if (animation)
	{
		animation->idleAnimation         = mount->savedIdleAnimation;
		animation->lockedOnIdleAnimation = mount->savedLockedOnIdleAnimation;
	}
	if (CharacterControllerComponent* beastController = entityManager.GetComponent<CharacterControllerComponent>(mountID))
	{
		beastController->moveDirection = glm::vec3(0.0f);
		beastController->wantsToRun    = false;
	}
	if (FollowCameraComponent* camera = FindFollowCamera(entityManager))
	{
		camera->pivotHeight -= mount->cameraRise;
		camera->distance    -= mount->cameraPullBack;
	}

	mount->isRidden = false;
	mount->riderID  = 0;
	SUNTA_ENGINE_LOG_INFO("Mount: '{0}' got off '{1}'", GetEntityName(entityManager, riderID), GetEntityName(entityManager, mountID));
}

void MountSystem::PrepareForPlay(EntityManager& entityManager)
{
	for (MountComponent& mount : entityManager.GetAllComponents<MountComponent>())
	{
		mount.isRidden = false;
		mount.riderID  = 0;
	}
}

void MountSystem::Update(EntityManager& entityManager, float deltaTime, const glm::vec3& cameraForward)
{
	(void)deltaTime;
	unsigned int playerID = 0;
	if (!CharacterControllerSystem::TryFindPlayer(entityManager, playerID))
		return;

	const TransformComponent* player = entityManager.GetComponent<TransformComponent>(playerID);
	if (!player)
		return;

	for (MountComponent& mount : entityManager.GetAllComponents<MountComponent>())
	{
		const unsigned int mountID = entityManager.GetEntityIDForComponent(mount);
		CharacterControllerComponent* steering = entityManager.GetComponent<CharacterControllerComponent>(mountID);
		const TransformComponent*     beast    = entityManager.GetComponent<TransformComponent>(mountID);
		if (!steering || !beast)
			continue;

		if (mount.isRidden)
		{
			if (mount.riderID != playerID)
				continue;
			const HealthComponent* health = entityManager.GetComponent<HealthComponent>(playerID);
			if ((health && health->isDead) || InputManager::IsKeyDown(MountKey))
			{
				Dismount(entityManager, mountID);
				continue;
			}
			HUDSystem::ShowPrompt(entityManager, mount.dismountPrompt);
			steering->moveDirection = ReadSteering(cameraForward);
			steering->wantsToRun    = InputManager::IsKeyPressed(GallopKey);
			continue;
		}

		const float distance = glm::length(KeepOnlyHorizontalPart(beast->position - player->position));
		if (distance > mount.mountDistance || !CanMount(entityManager, playerID) || IsRiding(entityManager, playerID))
			continue;

		HUDSystem::ShowPrompt(entityManager, mount.mountPrompt);
		if (InputManager::IsKeyDown(MountKey))
			Mount(entityManager, mount, mountID, playerID);
	}
}

void MountSystem::PlaceRiders(EntityManager& entityManager)
{
	for (MountComponent& mount : entityManager.GetAllComponents<MountComponent>())
	{
		if (!mount.isRidden)
			continue;

		const unsigned int mountID = entityManager.GetEntityIDForComponent(mount);
		const TransformComponent*   beast = entityManager.GetComponent<TransformComponent>(mountID);
		TransformComponent*         rider = entityManager.GetComponent<TransformComponent>(mount.riderID);
		const PhysicsBodyComponent* body  = entityManager.GetComponent<PhysicsBodyComponent>(mountID);
		if (!beast || !rider)
			continue;

		rider->position           = beast->position + beast->rotationQuaternion * (mount.seatOffset * beast->scale);
		rider->rotationQuaternion = beast->rotationQuaternion;
		rider->SyncEulerFromQuaternion();
		rider->isDirty = true;

		// The rider's idle is the gait of the mount (the rider itself stands still on its own body)
		if (CharacterAnimationComponent* animation = entityManager.GetComponent<CharacterAnimationComponent>(mount.riderID))
		{
			const float speed = body ? glm::length(KeepOnlyHorizontalPart(body->velocity)) : 0.0f;
			const std::string& gait = speed < 0.4f ? mount.riderIdleAnimation
			                        : speed < mount.runAnimationFromSpeed ? mount.riderWalkAnimation : mount.riderRunAnimation;
			animation->idleAnimation         = gait;
			animation->lockedOnIdleAnimation = gait;
		}
	}
}

bool MountSystem::IsRiding(EntityManager& entityManager, unsigned int riderID)
{
	for (MountComponent& mount : entityManager.GetAllComponents<MountComponent>())
		if (mount.isRidden && mount.riderID == riderID)
			return true;
	return false;
}

}
