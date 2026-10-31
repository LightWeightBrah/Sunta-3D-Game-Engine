#include "Core/SuntaPreCompiled.h"
#include "CharacterSetup.h"

#include "CharacterActionsSystem.h"
#include "CharacterAnimationSystem.h"
#include "CharacterControllerSystem.h"
#include "Core/EngineAssets.h"
#include "Core/Log.h"
#include "Core/ResourceManager.h"
#include "ECS/Component.h"
#include "ECS/EntityManager.h"
#include "Physics/CollisionSettings.h"
#include "Renderer/Model.h"
#include "Scene/EntityHierarchy.h"

namespace Sunta
{

namespace
{

// The bone of the Knight placed exactly where a sword sits in its right hand (see Tools/Blender/knight_generator.py)
constexpr const char* KnightWeaponBone = "Weapon_R";
constexpr const char* SwordName        = "Sword";

template<typename T>
bool Has(EntityManager& entityManager, unsigned int entityID)
{
	return entityManager.GetComponent<T>(entityID) != nullptr;
}

bool ModelHasBone(EntityManager& entityManager, unsigned int entityID, const std::string& boneName)
{
	const ModelComponent* model = entityManager.GetComponent<ModelComponent>(entityID);

	unsigned int nodeIndex = 0;
	return model && model->modelData && model->modelData->model && model->modelData->model->TryGetNodeIndex(boneName, nodeIndex);
}

bool HoldsSomethingOnBone(EntityManager& entityManager, unsigned int entityID, const std::string& boneName)
{
	for (unsigned int childID : EntityHierarchy::GetChildren(entityManager, entityID))
	{
		if (entityManager.GetComponent<ParentComponent>(childID)->boneName == boneName)
			return true;
	}

	return false;
}

bool TryFindLayer(const std::string& layerName, CollisionLayer& outLayer)
{
	if (CollisionSettings::GetActive().GetLayers().TryFindByName(layerName, outLayer))
		return true;

	SUNTA_ENGINE_LOG_WARNING("Character setup: the project has no collision layer called '{0}' (Project > Collision Layers & Matrix...)", layerName);
	return false;
}

}

unsigned int CharacterSetup::CreateNamedEntity(EntityManager& entityManager, const std::string& name, const glm::vec3& position, const glm::vec3& rotation)
{
	const unsigned int entityID = entityManager.CreateEntity();

	entityManager.AddComponent<TagComponent>(entityID).name = name;

	TransformComponent& transform = entityManager.AddComponent<TransformComponent>(entityID, position);
	transform.rotation = rotation;
	transform.SyncQuaternionFromEuler();

	entityManager.AddComponent<WorldMatrixComponent>(entityID);

	return entityID;
}

void CharacterSetup::AddModelIfMissing(EntityManager& entityManager, unsigned int entityID, const std::string& modelName)
{
	if (Has<ModelComponent>(entityManager, entityID))
		return;

	ModelComponent& model = entityManager.AddComponent<ModelComponent>(entityID, modelName);
	model.modelData = ResourceManager::GetModelData(modelName);
}

void CharacterSetup::AddFighterComponents(EntityManager& entityManager, unsigned int entityID)
{
	if (!Has<CharacterControllerComponent>(entityManager, entityID))
		CharacterControllerSystem::AddWithRequiredComponents(entityManager, entityID);

	if (!Has<CharacterAnimationComponent>(entityManager, entityID))
		CharacterAnimationSystem::AddWithRequiredComponents(entityManager, entityID);

	if (!Has<CharacterActionsComponent>(entityManager, entityID))
		CharacterActionsSystem::AddWithRequiredComponents(entityManager, entityID);

	if (!Has<HealthComponent>(entityManager, entityID))
		entityManager.AddComponent<HealthComponent>(entityID);
}

// The sword is a child on the weapon bone with no offset: the bone already sits where the grip goes
void CharacterSetup::AddSwordIfPossible(EntityManager& entityManager, unsigned int entityID, const std::string& hitboxLayerName, float damage)
{
	if (!ModelHasBone(entityManager, entityID, KnightWeaponBone) || HoldsSomethingOnBone(entityManager, entityID, KnightWeaponBone))
		return;

	const unsigned int swordID = CreateNamedEntity(entityManager, SwordName, glm::vec3(0.0f), glm::vec3(0.0f));

	entityManager.AddComponent<ModelComponent>(swordID, EngineAssets::Models::KnightSword);

	ParentComponent& parent = entityManager.AddComponent<ParentComponent>(swordID);
	parent.parentID = entityID;
	parent.boneName = KnightWeaponBone;

	HitboxComponent& hitbox = entityManager.AddComponent<HitboxComponent>(swordID);
	hitbox.damage = damage;
	TryFindLayer(hitboxLayerName, hitbox.layer);
}

void CharacterSetup::SetBodyLayer(EntityManager& entityManager, unsigned int entityID, const std::string& layerName)
{
	if (BoxColliderComponent* body = entityManager.GetComponent<BoxColliderComponent>(entityID))
		TryFindLayer(layerName, body->layer);
}

}
