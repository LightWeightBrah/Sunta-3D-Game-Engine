#include "Core/SuntaPreCompiled.h"
#include "RustedKnightSetup.h"

#include "CharacterSetup.h"
#include "MeleeEnemy.h"
#include "Core/EngineAssets.h"
#include "ECS/Component.h"
#include "ECS/EntityManager.h"

namespace Sunta
{

namespace
{

// The default collision layers of a project: the enemy's body, and what the enemy's sword may hit
constexpr const char* EnemyBodyLayer   = "Enemy";
constexpr const char* EnemyWeaponLayer = "EnemyHitbox";

constexpr float SwordDamage = 20.0f;
constexpr float Health      = 120.0f;

// A bit slower than the player, so it can be outrun
constexpr float WalkSpeed = 1.6f;
constexpr float RunSpeed  = 4.2f;

template<typename T>
bool Has(EntityManager& entityManager, unsigned int entityID)
{
	return entityManager.GetComponent<T>(entityID) != nullptr;
}

void TuneCharacter(EntityManager& entityManager, unsigned int entityID)
{
	CharacterControllerComponent& controller = *entityManager.GetComponent<CharacterControllerComponent>(entityID);
	controller.walkSpeed = WalkSpeed;
	controller.runSpeed  = RunSpeed;

	HealthComponent& health = *entityManager.GetComponent<HealthComponent>(entityID);
	health.maxHealth     = Health;
	health.currentHealth = Health;
}

}

void RustedKnightSetup::SetUp(EntityManager& entityManager, unsigned int entityID)
{
	if (!Has<TransformComponent>(entityManager, entityID) || IsSetUp(entityManager, entityID))
		return;

	CharacterSetup::AddModelIfMissing(entityManager, entityID, EngineAssets::Models::RustedKnight);
	CharacterSetup::AddFighterComponents(entityManager, entityID);
	MeleeEnemy::AddWithRequiredComponents(entityManager, entityID);
	TuneCharacter(entityManager, entityID);

	if (!Has<LockOnTargetComponent>(entityManager, entityID))
		entityManager.AddComponent<LockOnTargetComponent>(entityID);

	CharacterSetup::SetBodyLayer(entityManager, entityID, EnemyBodyLayer);
	CharacterSetup::AddSwordIfPossible(entityManager, entityID, EnemyWeaponLayer, SwordDamage);
}

bool RustedKnightSetup::IsSetUp(EntityManager& entityManager, unsigned int entityID)
{
	return Has<EnemyComponent>(entityManager, entityID);
}

}
