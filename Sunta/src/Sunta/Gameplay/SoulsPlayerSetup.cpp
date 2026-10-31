#include "Core/SuntaPreCompiled.h"
#include "SoulsPlayerSetup.h"

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include "CharacterSetup.h"
#include "FollowCameraSystem.h"
#include "Core/EngineAssets.h"
#include "ECS/Component.h"
#include "ECS/EntityManager.h"

namespace Sunta
{

namespace
{

// ========== Numbers used below ==========

// The default collision layers of a project: the player's body, and what the player's sword may hit
constexpr const char* PlayerBodyLayer   = "Player";
constexpr const char* PlayerWeaponLayer = "PlayerHitbox";

constexpr float PlayerSwordDamage = 30.0f;

constexpr const char* PlayerCameraName = "Player Camera";

// Where a new camera is put relative to the player (behind and above it, looking slightly down)
const glm::vec3 CameraOffsetBehindPlayer = glm::vec3(0.0f, 2.2f, 4.5f);
constexpr float CameraPitchDegrees       = -20.0f;

template<typename T>
bool Has(EntityManager& entityManager, unsigned int entityID)
{
	return entityManager.GetComponent<T>(entityID) != nullptr;
}

// ========== The player's own parts ==========

// Stamina limits rolling and attacking, Lock On aims at enemies, Lock On Target lets the enemies face the player
void AddPlayerComponents(EntityManager& entityManager, unsigned int entityID)
{
	if (!Has<StaminaComponent>(entityManager, entityID))
		entityManager.AddComponent<StaminaComponent>(entityID);

	if (!Has<LockOnComponent>(entityManager, entityID))
		entityManager.AddComponent<LockOnComponent>(entityID);

	if (!Has<LockOnTargetComponent>(entityManager, entityID))
		entityManager.AddComponent<LockOnTargetComponent>(entityID);
}

// ========== The camera ==========

bool SceneHasFollowCamera(EntityManager& entityManager)
{
	return !entityManager.GetAllComponents<FollowCameraComponent>().empty();
}

// The camera the game would use now: the first active one (another entity than the player)
bool TryFindSceneCamera(EntityManager& entityManager, unsigned int playerID, unsigned int& outEntityID)
{
	for (CameraComponent& camera : entityManager.GetAllComponents<CameraComponent>())
	{
		const unsigned int entityID = entityManager.GetEntityIDForComponent(camera);

		if (camera.isActive && entityID != playerID)
		{
			outEntityID = entityID;
			return true;
		}
	}

	return false;
}

unsigned int CreateCameraBehindPlayer(EntityManager& entityManager, unsigned int playerID)
{
	const TransformComponent& player = *entityManager.GetComponent<TransformComponent>(playerID);

	const glm::quat playerYaw = glm::quat(glm::radians(glm::vec3(0.0f, player.rotation.y, 0.0f)));
	const glm::vec3 position  = player.position + playerYaw * CameraOffsetBehindPlayer;

	return CharacterSetup::CreateNamedEntity(entityManager, PlayerCameraName, position, glm::vec3(CameraPitchDegrees, player.rotation.y, 0.0f));
}

void AddFollowCameraIfMissing(EntityManager& entityManager, unsigned int playerID)
{
	if (SceneHasFollowCamera(entityManager))
		return;

	unsigned int cameraID = 0;
	if (!TryFindSceneCamera(entityManager, playerID, cameraID))
		cameraID = CreateCameraBehindPlayer(entityManager, playerID);

	FollowCameraSystem::AddWithRequiredComponents(entityManager, cameraID);
}

}

void SoulsPlayerSetup::SetUp(EntityManager& entityManager, unsigned int entityID)
{
	// The sword and the camera are placed around the player, so it must stand somewhere
	if (!Has<TransformComponent>(entityManager, entityID))
		return;

	CharacterSetup::AddModelIfMissing(entityManager, entityID, EngineAssets::Models::Knight);
	CharacterSetup::AddFighterComponents(entityManager, entityID);
	AddPlayerComponents(entityManager, entityID);

	CharacterSetup::SetBodyLayer(entityManager, entityID, PlayerBodyLayer);
	CharacterSetup::AddSwordIfPossible(entityManager, entityID, PlayerWeaponLayer, PlayerSwordDamage);

	AddFollowCameraIfMissing(entityManager, entityID);
}

bool SoulsPlayerSetup::IsSetUp(EntityManager& entityManager, unsigned int entityID)
{
	return Has<CharacterActionsComponent>(entityManager, entityID) && Has<CharacterControllerComponent>(entityManager, entityID)
		&& entityManager.GetComponent<CharacterControllerComponent>(entityID)->isPlayerControlled;
}

}
