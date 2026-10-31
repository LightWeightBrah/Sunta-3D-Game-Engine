#include "Core/SuntaPreCompiled.h"
#include "CombatSystem.h"

#include <algorithm>

#include "CharacterActionsSystem.h"
#include "EntityName.h"
#include "Core/Log.h"
#include "ECS/Component.h"
#include "ECS/EntityManager.h"
#include "Physics/CollisionSettings.h"
#include "Physics/CollisionShapes.h"
#include "Physics/SAT.h"
#include "Scene/EntityHierarchy.h"

namespace Sunta
{

namespace
{

// How long a weapon's box stays red after it hurt someone
constexpr float HitFlashSeconds = 0.4f;

// ========== The owner of a weapon ==========

// The closest entity with Character Actions, going up from the weapon through its parents (the hand holding it)
bool TryFindWeaponOwner(EntityManager& entityManager, unsigned int weaponEntityID, unsigned int& outOwnerID)
{
	unsigned int entityID = weaponEntityID;

	while (!entityManager.GetComponent<CharacterActionsComponent>(entityID))
	{
		if (!EntityHierarchy::TryGetParent(entityManager, entityID, entityID))
			return false;
	}

	outOwnerID = entityID;
	return true;
}

// The Character Actions of the owner while it is in the striking part of an attack, nullptr otherwise
const CharacterActionsComponent* GetStrikingOwner(EntityManager& entityManager, unsigned int weaponEntityID, unsigned int& outOwnerID)
{
	if (!TryFindWeaponOwner(entityManager, weaponEntityID, outOwnerID))
		return nullptr;

	const CharacterActionsComponent* actions = entityManager.GetComponent<CharacterActionsComponent>(outOwnerID);
	return actions->IsInHitWindow() ? actions : nullptr;
}

// ========== Hitting ==========

OBB GetWorldBox(EntityManager& entityManager, unsigned int weaponEntityID, const HitboxComponent& hitbox)
{
	const WorldMatrixComponent* worldMatrix = entityManager.GetComponent<WorldMatrixComponent>(weaponEntityID);
	const glm::mat4 matrix = worldMatrix ? worldMatrix->matrix : glm::mat4(1.0f);

	return MakeWorldOBB(matrix, hitbox.localOffset, hitbox.halfExtents);
}

// A new swing may hit the same enemies again
void StartNewSwingIfNeeded(HitboxComponent& hitbox, const CharacterActionsComponent& owner)
{
	if (hitbox.swingNumber == owner.attackNumber)
		return;

	hitbox.swingNumber = owner.attackNumber;
	hitbox.entitiesHitThisSwing.clear();
}

bool WasHitThisSwing(const HitboxComponent& hitbox, unsigned int targetEntityID)
{
	const std::vector<unsigned int>& hits = hitbox.entitiesHitThisSwing;
	return std::find(hits.begin(), hits.end(), targetEntityID) != hits.end();
}

// The collision rules decide who can hurt whom (PlayerHitbox + Enemy, EnemyHitbox + Player)
bool CanHurt(const HitboxComponent& hitbox, const BoxColliderComponent& targetBody)
{
	return CollisionSettings::GetActive().CanCollide(hitbox.layer, targetBody.layer);
}

void HitTargetsInReach(EntityManager& entityManager, HitboxComponent& hitbox, unsigned int weaponEntityID,
	unsigned int ownerID, const CharacterActionsComponent& owner)
{
	const OBB   box    = GetWorldBox(entityManager, weaponEntityID, hitbox);
	const float damage = hitbox.damage * owner.GetDamageMultiplier();

	for (HealthComponent& health : entityManager.GetAllComponents<HealthComponent>())
	{
		const unsigned int targetID = entityManager.GetEntityIDForComponent(health);
		const BoxColliderComponent* targetBody = entityManager.GetComponent<BoxColliderComponent>(targetID);

		if (targetID == ownerID || health.isDead || !targetBody || WasHitThisSwing(hitbox, targetID))
			continue;

		if (!CanHurt(hitbox, *targetBody) || !Overlaps(box, targetBody->worldOBB))
			continue;

		hitbox.entitiesHitThisSwing.push_back(targetID);

		if (CombatSystem::ApplyDamage(entityManager, targetID, damage, ownerID))
			hitbox.hitFlashSecondsLeft = HitFlashSeconds;
	}
}

// ========== Getting hurt ==========

bool IsInvulnerable(EntityManager& entityManager, unsigned int entityID)
{
	const CharacterActionsComponent* actions = entityManager.GetComponent<CharacterActionsComponent>(entityID);
	return actions && actions->isInvulnerable;
}

void Die(EntityManager& entityManager, unsigned int entityID, HealthComponent& health)
{
	health.isDead = true;

	// Nobody keeps the lock on a dead body
	if (LockOnTargetComponent* lockOnTarget = entityManager.GetComponent<LockOnTargetComponent>(entityID))
		lockOnTarget->isLockable = false;

	CharacterActionsSystem::PlayDeath(entityManager, entityID);
}

}

void CombatSystem::PrepareForPlay(EntityManager& entityManager)
{
	for (HealthComponent& health : entityManager.GetAllComponents<HealthComponent>())
	{
		health.currentHealth = health.maxHealth;
		health.isDead        = false;
	}
}

void CombatSystem::Update(EntityManager& entityManager, float deltaTime)
{
	for (HitboxComponent& hitbox : entityManager.GetAllComponents<HitboxComponent>())
	{
		hitbox.hitFlashSecondsLeft = glm::max(hitbox.hitFlashSecondsLeft - deltaTime, 0.0f);

		const unsigned int weaponEntityID = entityManager.GetEntityIDForComponent(hitbox);

		unsigned int ownerID = 0;
		const CharacterActionsComponent* owner = GetStrikingOwner(entityManager, weaponEntityID, ownerID);
		if (!owner)
			continue;

		StartNewSwingIfNeeded(hitbox, *owner);
		HitTargetsInReach(entityManager, hitbox, weaponEntityID, ownerID, *owner);
	}
}

bool CombatSystem::ApplyDamage(EntityManager& entityManager, unsigned int targetEntityID, float damage, unsigned int attackerEntityID)
{
	HealthComponent* health = entityManager.GetComponent<HealthComponent>(targetEntityID);

	if (!health || health->isDead || IsInvulnerable(entityManager, targetEntityID))
		return false;

	health->currentHealth = glm::max(health->currentHealth - damage, 0.0f);

	SUNTA_ENGINE_LOG_INFO("Combat: '{0}' hit '{1}' for {2} ({3} health left)", GetEntityName(entityManager, attackerEntityID),
		GetEntityName(entityManager, targetEntityID), damage, health->currentHealth);

	if (health->currentHealth <= 0.0f)
		Die(entityManager, targetEntityID, *health);
	else
		CharacterActionsSystem::PlayHitReaction(entityManager, targetEntityID);

	return true;
}

bool CombatSystem::HasJustHit(const HitboxComponent& hitbox)
{
	return hitbox.hitFlashSecondsLeft > 0.0f;
}

void CombatSystem::ReportProblemsBeforePlay(EntityManager& entityManager)
{
	for (HitboxComponent& hitbox : entityManager.GetAllComponents<HitboxComponent>())
	{
		const unsigned int weaponEntityID = entityManager.GetEntityIDForComponent(hitbox);

		unsigned int ownerID = 0;
		if (!TryFindWeaponOwner(entityManager, weaponEntityID, ownerID))
			SUNTA_ENGINE_LOG_WARNING("Hitbox on '{0}' has no owner with Character Actions (make the weapon a child of the character), it will never hurt",
				GetEntityName(entityManager, weaponEntityID));
	}

	for (HealthComponent& health : entityManager.GetAllComponents<HealthComponent>())
	{
		const unsigned int entityID = entityManager.GetEntityIDForComponent(health);

		if (!entityManager.GetComponent<BoxColliderComponent>(entityID))
			SUNTA_ENGINE_LOG_WARNING("Health on '{0}': without a Box Collider no weapon can reach it", GetEntityName(entityManager, entityID));
	}
}

}
