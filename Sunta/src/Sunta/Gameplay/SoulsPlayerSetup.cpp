#include "Core/SuntaPreCompiled.h"
#include "SoulsPlayerSetup.h"

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include "CharacterActionsSystem.h"
#include "CharacterAnimationSystem.h"
#include "CharacterControllerSystem.h"
#include "FollowCameraSystem.h"
#include "Core/EngineAssets.h"
#include "Core/ResourceManager.h"
#include "ECS/Component.h"
#include "ECS/EntityManager.h"
#include "Renderer/Model.h"
#include "Scene/EntityHierarchy.h"

namespace Sunta
{

namespace
{

// ========== Numbers used below ==========

// The bone of the Knight placed exactly where a sword sits in its right hand (see Tools/Blender/knight_generator.py)
constexpr const char* KnightWeaponBone = "Weapon_R";

constexpr const char* SwordName        = "Sword";
constexpr const char* PlayerCameraName = "Player Camera";

// Where a new camera is put relative to the player (behind and above it, looking slightly down)
const glm::vec3 CameraOffsetBehindPlayer = glm::vec3(0.0f, 2.2f, 4.5f);
constexpr float CameraPitchDegrees       = -20.0f;

// ========== Small helpers ==========

unsigned int CreateNamedEntity(EntityManager& entityManager, const std::string& name, const glm::vec3& position, const glm::vec3& rotation)
{
	const unsigned int entityID = entityManager.CreateEntity();

	entityManager.AddComponent<TagComponent>(entityID).name = name;

	TransformComponent& transform = entityManager.AddComponent<TransformComponent>(entityID, position);
	transform.rotation = rotation;
	transform.SyncQuaternionFromEuler();

	entityManager.AddComponent<WorldMatrixComponent>(entityID);

	return entityID;
}

template<typename T>
bool Has(EntityManager& entityManager, unsigned int entityID)
{
	return entityManager.GetComponent<T>(entityID) != nullptr;
}

// ========== The character ==========

// The model is loaded at once (not on the next frame), so the components added after it can pick its animations
void AddKnightModelIfMissing(EntityManager& entityManager, unsigned int entityID)
{
	if (Has<ModelComponent>(entityManager, entityID))
		return;

	ModelComponent& model = entityManager.AddComponent<ModelComponent>(entityID, EngineAssets::Models::Knight);
	model.modelData = ResourceManager::GetModelData(EngineAssets::Models::Knight);
}

void AddCharacterComponents(EntityManager& entityManager, unsigned int entityID)
{
	if (!Has<CharacterControllerComponent>(entityManager, entityID))
		CharacterControllerSystem::AddWithRequiredComponents(entityManager, entityID);

	if (!Has<CharacterAnimationComponent>(entityManager, entityID))
		CharacterAnimationSystem::AddWithRequiredComponents(entityManager, entityID);

	if (!Has<CharacterActionsComponent>(entityManager, entityID))
		CharacterActionsSystem::AddWithRequiredComponents(entityManager, entityID);

	if (!Has<LockOnComponent>(entityManager, entityID))
		entityManager.AddComponent<LockOnComponent>(entityID);
}

// ========== The sword ==========

bool ModelHasBone(EntityManager& entityManager, unsigned int entityID, const std::string& boneName)
{
	const ModelComponent* model = entityManager.GetComponent<ModelComponent>(entityID);

	unsigned int nodeIndex = 0;
	return model && model->modelData && model->modelData->model && model->modelData->model->TryGetNodeIndex(boneName, nodeIndex);
}

bool HoldsSomethingOnBone(EntityManager& entityManager, unsigned int playerID, const std::string& boneName)
{
	for (unsigned int childID : EntityHierarchy::GetChildren(entityManager, playerID))
	{
		if (entityManager.GetComponent<ParentComponent>(childID)->boneName == boneName)
			return true;
	}

	return false;
}

// The sword is a child of the player on the weapon bone, with no offset: the bone already sits where the grip goes
// Only a model with the Knight's weapon bone gets it, on another model nobody knows where the hand is
void AddSwordIfPossible(EntityManager& entityManager, unsigned int playerID)
{
	if (!ModelHasBone(entityManager, playerID, KnightWeaponBone) || HoldsSomethingOnBone(entityManager, playerID, KnightWeaponBone))
		return;

	const unsigned int swordID = CreateNamedEntity(entityManager, SwordName, glm::vec3(0.0f), glm::vec3(0.0f));

	entityManager.AddComponent<ModelComponent>(swordID, EngineAssets::Models::KnightSword);

	ParentComponent& parent = entityManager.AddComponent<ParentComponent>(swordID);
	parent.parentID = playerID;
	parent.boneName = KnightWeaponBone;
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

	return CreateNamedEntity(entityManager, PlayerCameraName, position, glm::vec3(CameraPitchDegrees, player.rotation.y, 0.0f));
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

	AddKnightModelIfMissing(entityManager, entityID);
	AddCharacterComponents(entityManager, entityID);
	AddSwordIfPossible(entityManager, entityID);
	AddFollowCameraIfMissing(entityManager, entityID);
}

bool SoulsPlayerSetup::IsSetUp(EntityManager& entityManager, unsigned int entityID)
{
	return Has<CharacterActionsComponent>(entityManager, entityID);
}

}
