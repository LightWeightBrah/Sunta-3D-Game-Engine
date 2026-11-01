#include "Core/SuntaPreCompiled.h"
#include "CombatSystem.h"

#include <algorithm>
#include <cmath>

#include "CharacterActionsSystem.h"
#include "EntityName.h"
#include "GameplayMath.h"
#include "StaminaSystem.h"
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

using namespace GameplayMath;

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

bool HitWithWeapon(EntityManager& entityManager, unsigned int targetID, float damage, float poiseDamage, unsigned int attackerID);

void HitTargetsInReach(EntityManager& entityManager, HitboxComponent& hitbox, unsigned int weaponEntityID,
	unsigned int ownerID, const CharacterActionsComponent& owner)
{
	const OBB   box         = GetWorldBox(entityManager, weaponEntityID, hitbox);
	const float damage      = hitbox.damage      * owner.GetDamageMultiplier();
	const float poiseDamage = hitbox.poiseDamage * owner.GetDamageMultiplier();

	for (HealthComponent& health : entityManager.GetAllComponents<HealthComponent>())
	{
		const unsigned int targetID = entityManager.GetEntityIDForComponent(health);
		const BoxColliderComponent* targetBody = entityManager.GetComponent<BoxColliderComponent>(targetID);

		if (targetID == ownerID || health.isDead || !targetBody || WasHitThisSwing(hitbox, targetID))
			continue;

		if (!CanHurt(hitbox, *targetBody) || !Overlaps(box, targetBody->worldOBB))
			continue;

		hitbox.entitiesHitThisSwing.push_back(targetID);

		if (HitWithWeapon(entityManager, targetID, damage, poiseDamage, ownerID))
			hitbox.hitFlashSecondsLeft = HitFlashSeconds;
	}
}

// ========== Getting hurt ==========

bool IsInvulnerable(EntityManager& entityManager, unsigned int entityID)
{
	const CharacterActionsComponent* actions = entityManager.GetComponent<CharacterActionsComponent>(entityID);
	return actions && actions->isInvulnerable;
}

bool CanBeHurt(EntityManager& entityManager, unsigned int entityID)
{
	const HealthComponent* health = entityManager.GetComponent<HealthComponent>(entityID);
	return health && !health->isDead && !IsInvulnerable(entityManager, entityID);
}

void Die(EntityManager& entityManager, unsigned int entityID, HealthComponent& health)
{
	health.isDead = true;

	// Nobody keeps the lock on a dead body
	if (LockOnTargetComponent* lockOnTarget = entityManager.GetComponent<LockOnTargetComponent>(entityID))
		lockOnTarget->isLockable = false;

	CharacterActionsSystem::PlayDeath(entityManager, entityID);
}

// Takes the damage away from the health. True when the character is still alive
bool LoseHealth(EntityManager& entityManager, unsigned int entityID, float damage, unsigned int attackerEntityID)
{
	HealthComponent& health = *entityManager.GetComponent<HealthComponent>(entityID);
	health.currentHealth = glm::max(health.currentHealth - damage, 0.0f);

	SUNTA_ENGINE_LOG_INFO("Combat: '{0}' hit '{1}' for {2} ({3} health left)", GetEntityName(entityManager, attackerEntityID),
		GetEntityName(entityManager, entityID), damage, health.currentHealth);

	if (health.currentHealth > 0.0f)
		return true;

	Die(entityManager, entityID, health);
	return false;
}

// ========== Poise ==========

// True when the hit makes the character stagger: a character without Poise always staggers,
// with Poise only when the hit takes the last of it (and then the poise is full again)
bool BreaksPoise(EntityManager& entityManager, unsigned int entityID, float poiseDamage)
{
	PoiseComponent* poise = entityManager.GetComponent<PoiseComponent>(entityID);
	if (!poise)
		return true;

	poise->secondsSinceHit = 0.0f;
	poise->currentPoise   -= poiseDamage;

	if (poise->currentPoise > 0.0f)
		return false;

	poise->currentPoise = poise->maxPoise;
	return true;
}

void RecoverPoise(PoiseComponent& poise, float deltaTime)
{
	poise.secondsSinceHit += deltaTime;

	if (poise.secondsSinceHit >= poise.recoverAfterSeconds)
		poise.currentPoise = poise.maxPoise;
}

// ========== Parrying and blocking ==========

const CharacterActionsComponent* GetActions(EntityManager& entityManager, unsigned int entityID)
{
	return entityManager.GetComponent<CharacterActionsComponent>(entityID);
}

// The attack lands early in the defender's parry: the attacker staggers for long, nobody is hurt
bool TryParry(EntityManager& entityManager, unsigned int defenderID, unsigned int attackerID)
{
	const CharacterActionsComponent* defender = GetActions(entityManager, defenderID);
	if (!defender || !defender->IsParrying())
		return false;

	CharacterActionsSystem::PlayParried(entityManager, attackerID);

	SUNTA_ENGINE_LOG_INFO("Combat: '{0}' parried '{1}'", GetEntityName(entityManager, defenderID), GetEntityName(entityManager, attackerID));
	return true;
}

// The shield of the character: a child entity with a Shield component (on the left hand)
const ShieldComponent* FindShield(EntityManager& entityManager, unsigned int characterID)
{
	for (unsigned int childID : EntityHierarchy::GetChildren(entityManager, characterID))
	{
		if (const ShieldComponent* shield = entityManager.GetComponent<ShieldComponent>(childID))
			return shield;
	}

	return nullptr;
}

// The attacker stands inside the arc the shield covers, in front of the defender
bool IsInFrontOfShield(EntityManager& entityManager, unsigned int defenderID, unsigned int attackerID, const ShieldComponent& shield)
{
	const TransformComponent* defender = entityManager.GetComponent<TransformComponent>(defenderID);
	const TransformComponent* attacker = entityManager.GetComponent<TransformComponent>(attackerID);
	if (!defender || !attacker)
		return false;

	const CharacterControllerComponent* controller = entityManager.GetComponent<CharacterControllerComponent>(defenderID);
	const float facingOffset = controller ? controller->facingOffsetDegrees : 0.0f;

	const glm::vec3 facing     = GetDirectionFromYawAndPitch(defender->rotation.y - facingOffset, 0.0f);
	const glm::vec3 toAttacker = GetHorizontalDirectionOrDefault(attacker->position - defender->position);

	return glm::dot(facing, toAttacker) >= std::cos(glm::radians(shield.guardAngleDegrees * 0.5f));
}

enum class BlockResult
{
	NotBlocked,  // no shield up, or the hit came from the side or the back
	Blocked,     // the shield took the hit
	GuardBroken  // the shield was up, but there was not enough stamina to hold it
};

// A blocked hit costs stamina and hurts only by the part the shield lets through ('inOutDamage' is lowered to it)
BlockResult TryBlock(EntityManager& entityManager, unsigned int defenderID, unsigned int attackerID, float& inOutDamage)
{
	const CharacterActionsComponent* defender = GetActions(entityManager, defenderID);
	const ShieldComponent*           shield   = FindShield(entityManager, defenderID);

	if (!defender || !defender->IsGuarding() || !shield || !IsInFrontOfShield(entityManager, defenderID, attackerID, *shield))
		return BlockResult::NotBlocked;

	if (!StaminaSystem::TrySpend(entityManager, defenderID, inOutDamage * shield->staminaPerDamage))
	{
		SUNTA_ENGINE_LOG_INFO("Combat: the guard of '{0}' broke", GetEntityName(entityManager, defenderID));
		return BlockResult::GuardBroken;
	}

	SUNTA_ENGINE_LOG_INFO("Combat: '{0}' blocked '{1}' with the shield", GetEntityName(entityManager, defenderID), GetEntityName(entityManager, attackerID));

	inOutDamage *= shield->damageThrough;
	return BlockResult::Blocked;
}

// A character staggering after its attack was parried takes a riposte: much more damage
float GetRiposteMultiplier(EntityManager& entityManager, unsigned int defenderID, unsigned int attackerID)
{
	const CharacterActionsComponent* defender = GetActions(entityManager, defenderID);
	const CharacterActionsComponent* attacker = GetActions(entityManager, attackerID);

	return (defender && attacker && defender->IsOpenToRiposte()) ? attacker->riposteDamageMultiplier : 1.0f;
}

// ========== A weapon's hit ==========

// Goes through the target's defences: a parry turns it away, a shield takes it, poise keeps the target on its feet
// True when the hit landed (on the body or on the shield)
bool HitWithWeapon(EntityManager& entityManager, unsigned int targetID, float damage, float poiseDamage, unsigned int attackerID)
{
	if (!CanBeHurt(entityManager, targetID) || TryParry(entityManager, targetID, attackerID))
		return false;

	const BlockResult block = TryBlock(entityManager, targetID, attackerID, damage);

	if (block == BlockResult::Blocked)
	{
		if (damage <= 0.0f || LoseHealth(entityManager, targetID, damage, attackerID))
			CharacterActionsSystem::PlayBlockImpact(entityManager, targetID);
		return true;
	}

	damage *= GetRiposteMultiplier(entityManager, targetID, attackerID);

	const bool staggers = block == BlockResult::GuardBroken || BreaksPoise(entityManager, targetID, poiseDamage);

	if (LoseHealth(entityManager, targetID, damage, attackerID) && staggers)
		CharacterActionsSystem::PlayHitReaction(entityManager, targetID);

	return true;
}

}

void CombatSystem::PrepareForPlay(EntityManager& entityManager)
{
	for (HealthComponent& health : entityManager.GetAllComponents<HealthComponent>())
	{
		health.currentHealth = health.maxHealth;
		health.isDead        = false;
	}

	for (PoiseComponent& poise : entityManager.GetAllComponents<PoiseComponent>())
	{
		poise.currentPoise    = poise.maxPoise;
		poise.secondsSinceHit = 0.0f;
	}
}

namespace
{

// How many specks the shockwave's effect throws at once
constexpr int ShockwaveBurst = 140;

// Only targets within this height of the shockwave are hurt (not someone standing on a wall above)
constexpr float ShockwaveHeight = 2.0f;

bool IsOnPlayersSide(EntityManager& entityManager, unsigned int entityID)
{
	const CharacterControllerComponent* controller = entityManager.GetComponent<CharacterControllerComponent>(entityID);
	return controller && controller->isPlayerControlled;
}

void BurstEffect(EntityManager& entityManager, unsigned int ownerID, const std::string& effectName)
{
	if (effectName.empty())
		return;

	for (unsigned int childID : EntityHierarchy::GetChildren(entityManager, ownerID))
	{
		const TagComponent* tag = entityManager.GetComponent<TagComponent>(childID);
		ParticleEmitterComponent* emitter = entityManager.GetComponent<ParticleEmitterComponent>(childID);
		if (tag && emitter && tag->name == effectName)
			emitter->burstCount += ShockwaveBurst;
	}
}

// The moment a heavy attack or a leap starts to hit, once per attack
void UpdateShockwave(EntityManager& entityManager, unsigned int ownerID, CharacterActionsComponent& actions)
{
	if (actions.shockwaveRadius <= 0.0f || actions.shockwaveAttackNumber == actions.attackNumber)
		return;

	const bool isHeavy = actions.currentAction == CharacterAction::HeavyAttack && actions.actionProgress >= actions.heavyHitFrom;
	const bool isLeap  = actions.currentAction == CharacterAction::LeapAttack  && actions.actionProgress >= actions.leapHitFrom;
	if (!isHeavy && !isLeap)
		return;

	actions.shockwaveAttackNumber = actions.attackNumber;

	const TransformComponent*           transform  = entityManager.GetComponent<TransformComponent>(ownerID);
	const CharacterControllerComponent* controller = entityManager.GetComponent<CharacterControllerComponent>(ownerID);
	if (!transform || !controller)
		return;

	const glm::vec3 forward = GameplayMath::GetDirectionFromYawAndPitch(transform->rotation.y - controller->facingOffsetDegrees, 0.0f);
	const glm::vec3 center  = transform->position + forward * actions.shockwaveDistance;
	const bool      isPlayerSide = IsOnPlayersSide(entityManager, ownerID);

	BurstEffect(entityManager, ownerID, actions.shockwaveEffect);

	for (HealthComponent& health : entityManager.GetAllComponents<HealthComponent>())
	{
		const unsigned int targetID = entityManager.GetEntityIDForComponent(health);
		const TransformComponent* target = entityManager.GetComponent<TransformComponent>(targetID);
		if (targetID == ownerID || !target || IsOnPlayersSide(entityManager, targetID) == isPlayerSide)
			continue;

		const glm::vec3 offset = target->position - center;
		if (glm::length(glm::vec2(offset.x, offset.z)) <= actions.shockwaveRadius && std::abs(offset.y) <= ShockwaveHeight)
			CombatSystem::ApplyDamage(entityManager, targetID, actions.shockwaveDamage, ownerID);
	}
}

}

void CombatSystem::Update(EntityManager& entityManager, float deltaTime)
{
	for (CharacterActionsComponent& actions : entityManager.GetAllComponents<CharacterActionsComponent>())
		UpdateShockwave(entityManager, entityManager.GetEntityIDForComponent(actions), actions);

	for (PoiseComponent& poise : entityManager.GetAllComponents<PoiseComponent>())
		RecoverPoise(poise, deltaTime);

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
	if (!CanBeHurt(entityManager, targetEntityID))
		return false;

	if (LoseHealth(entityManager, targetEntityID, damage, attackerEntityID))
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
