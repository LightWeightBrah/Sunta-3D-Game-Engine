#include "Core/SuntaPreCompiled.h"
#include "BonfireSystem.h"

#include <limits>

#include "CharacterActionsSystem.h"
#include "CharacterControllerSystem.h"
#include "GameplayMath.h"
#include "HUDSystem.h"
#include "RespawnSystem.h"
#include "Core/InputManager.h"
#include "Core/KeyCodes.h"
#include "ECS/Component.h"
#include "ECS/EntityManager.h"

namespace Sunta
{

namespace
{

using namespace GameplayMath;

constexpr int RestKey = SUNTA_KEY_E;

constexpr const char* RestPrompt    = "Rest at the bonfire   [E]";
constexpr const char* StandUpPrompt = "Leave the bonfire   [E]";
constexpr const char* LitMessage = "BONFIRE LIT";

bool IsResting(EntityManager& entityManager, unsigned int playerID)
{
	const CharacterActionsComponent* actions = entityManager.GetComponent<CharacterActionsComponent>(playerID);
	return actions && actions->IsResting();
}

// Alive, controlled by the player and not in the middle of a roll, an attack or a rest
bool CanRest(EntityManager& entityManager, unsigned int playerID)
{
	const HealthComponent*           health  = entityManager.GetComponent<HealthComponent>(playerID);
	const CharacterActionsComponent* actions = entityManager.GetComponent<CharacterActionsComponent>(playerID);

	return actions && !actions->IsBusy() && !(health && health->isDead);
}

bool TryFindNearestBonfire(EntityManager& entityManager, const glm::vec3& playerPosition, unsigned int& outBonfireID)
{
	float nearestDistance = std::numeric_limits<float>::max();
	bool  isFound = false;

	for (BonfireComponent& bonfire : entityManager.GetAllComponents<BonfireComponent>())
	{
		const unsigned int bonfireID = entityManager.GetEntityIDForComponent(bonfire);
		const TransformComponent* transform = entityManager.GetComponent<TransformComponent>(bonfireID);
		if (!transform)
			continue;

		const float distance = glm::length(KeepOnlyHorizontalPart(transform->position - playerPosition));

		if (distance <= bonfire.interactDistance && distance < nearestDistance)
		{
			nearestDistance = distance;
			outBonfireID    = bonfireID;
			isFound         = true;
		}
	}

	return isFound;
}

// The yaw that turns the character towards a point (with the facing offset of its model)
float GetYawTowards(EntityManager& entityManager, unsigned int characterID, const glm::vec3& from, const glm::vec3& to)
{
	const CharacterControllerComponent* controller = entityManager.GetComponent<CharacterControllerComponent>(characterID);
	const float facingOffset = controller ? controller->facingOffsetDegrees : 0.0f;

	return WrapDegrees(GetYawToFace(KeepOnlyHorizontalPart(to - from)) + facingOffset);
}

void FacePoint(EntityManager& entityManager, unsigned int characterID, const glm::vec3& point)
{
	TransformComponent& transform = *entityManager.GetComponent<TransformComponent>(characterID);

	transform.rotation = glm::vec3(0.0f, GetYawTowards(entityManager, characterID, transform.position, point), 0.0f);
	transform.SyncQuaternionFromEuler();
	transform.isDirty = true;
}

// The player comes back in front of the bonfire, facing it
void RememberAsRespawnPoint(EntityManager& entityManager, unsigned int playerID, const TransformComponent& bonfireTransform, const BonfireComponent& bonfire)
{
	const glm::vec3 point = bonfireTransform.position + bonfireTransform.rotationQuaternion * bonfire.respawnOffset;
	RespawnSystem::SetRespawnPoint(entityManager, playerID, point, GetYawTowards(entityManager, playerID, point, bonfireTransform.position));
}

void Rest(EntityManager& entityManager, unsigned int playerID, unsigned int bonfireID)
{
	BonfireComponent&         bonfire          = *entityManager.GetComponent<BonfireComponent>(bonfireID);
	const TransformComponent& bonfireTransform = *entityManager.GetComponent<TransformComponent>(bonfireID);

	FacePoint(entityManager, playerID, bonfireTransform.position);
	CharacterActionsSystem::PlayRest(entityManager, playerID);

	RespawnSystem::RestoreCharacter(entityManager, playerID);
	RememberAsRespawnPoint(entityManager, playerID, bonfireTransform, bonfire);
	RespawnSystem::RespawnEnemies(entityManager);

	if (!bonfire.isLit)
	{
		bonfire.isLit = true;
		HUDSystem::ShowNotification(entityManager, LitMessage);
	}
}

// A resting player stays kneeling until E is pressed again
void OfferToStandUp(EntityManager& entityManager, unsigned int playerID)
{
	HUDSystem::ShowPrompt(entityManager, StandUpPrompt);

	if (InputManager::IsKeyDown(RestKey))
		CharacterActionsSystem::StandUp(entityManager, playerID);
}

void OfferToRest(EntityManager& entityManager, unsigned int playerID)
{
	if (!CanRest(entityManager, playerID))
		return;

	const TransformComponent* playerTransform = entityManager.GetComponent<TransformComponent>(playerID);

	unsigned int bonfireID = 0;
	if (!playerTransform || !TryFindNearestBonfire(entityManager, playerTransform->position, bonfireID))
		return;

	HUDSystem::ShowPrompt(entityManager, RestPrompt);

	if (InputManager::IsKeyDown(RestKey))
		Rest(entityManager, playerID, bonfireID);
}

}

void BonfireSystem::PrepareForPlay(EntityManager& entityManager)
{
	for (BonfireComponent& bonfire : entityManager.GetAllComponents<BonfireComponent>())
		bonfire.isLit = false;
}

void BonfireSystem::Update(EntityManager& entityManager)
{
	unsigned int playerID = 0;
	if (!CharacterControllerSystem::TryFindPlayer(entityManager, playerID))
		return;

	if (IsResting(entityManager, playerID))
		OfferToStandUp(entityManager, playerID);
	else
		OfferToRest(entityManager, playerID);
}

}
