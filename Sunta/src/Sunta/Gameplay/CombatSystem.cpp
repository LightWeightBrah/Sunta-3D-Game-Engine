#include "Core/SuntaPreCompiled.h"
#include "CombatSystem.h"
#include "Renderer/PostProcessing.h"
#include "DeveloperCheats.h"
#include "Effects/EffectSystem.h"

#include <unordered_map>

#include <algorithm>
#include <cmath>

#include "CharacterActionsSystem.h"
#include "CombatLog.h"
#include "CharacterSoundsSystem.h"
#include "CurrencySystem.h"
#include "EntityName.h"
#include "GameplayMath.h"
#include "LevelUpSystem.h"
#include "FollowCameraSystem.h"
#include "StaminaSystem.h"
#include "TimeScaleSystem.h"
#include "Core/Log.h"
#include "ECS/Component.h"
#include "ECS/EntityManager.h"
#include "Effects/ParticleSystem.h"
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

// ========== How a hit feels (hit stop, camera shake, sparks) ==========

// How many specks the hit effects throw where the weapon landed
constexpr int HitEffectBurst   = 24;
constexpr int BlockEffectBurst = 18;
constexpr int ParryEffectBurst = 60;

// A heavy blow (and a leap) freezes and shakes this much more than a light one
constexpr float HeavyFeelMultiplier = 1.5f;

// A blow on a shield feels a little softer than one on the body
constexpr float BlockedFeelMultiplier = 0.7f;
// A blow glancing off a protected body: a short jolt, less than a block
constexpr float DeflectedFeelMultiplier = 0.4f;
constexpr int   DeflectEffectBurst      = 10;

// A parry and a riposte are the big moments of a duel: a long freeze and a strong shake
constexpr float ParryHitStopSeconds   = 0.16f;
constexpr float ParryCameraShake      = 0.35f;
constexpr float RiposteHitStopSeconds = 0.2f;
constexpr float RiposteCameraShake    = 0.5f;

// What a weapon's hit came to
enum class HitResult
{
	Missed,   // the target couldn't be hurt (dead, rolling)
	Parried,  // turned away by a parry: nobody is hurt, the attacker staggers
	Blocked,  // the shield took it (or the guard broke)
	Hurt,     // it landed on the body
	Riposte   // it landed on a parried attacker: a critical blow
};

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

// A character that strikes with several Hitboxes at once (both fists and a foot, a weapon in each hand) hurts
// a target once per attack, whichever of them touches it first
struct AttackHits
{
	unsigned int              attackNumber = 0;
	std::vector<unsigned int> targets;
};

std::unordered_map<unsigned int, AttackHits>& GetAttackHits()
{
	static std::unordered_map<unsigned int, AttackHits> hitsOfOwners;
	return hitsOfOwners;
}

// True the first time this attack of the owner reaches the target
bool TryClaimHit(unsigned int ownerID, unsigned int attackNumber, unsigned int targetID)
{
	AttackHits& hits = GetAttackHits()[ownerID];
	if (hits.attackNumber != attackNumber)
	{
		hits.attackNumber = attackNumber;
		hits.targets.clear();
	}

	if (std::find(hits.targets.begin(), hits.targets.end(), targetID) != hits.targets.end())
		return false;

	hits.targets.push_back(targetID);
	return true;
}

// The collision rules decide who can hurt whom (PlayerHitbox + Enemy, EnemyHitbox + Player)
bool CanHurt(const HitboxComponent& hitbox, const BoxColliderComponent& targetBody)
{
	return CollisionSettings::GetActive().CanCollide(hitbox.layer, targetBody.layer);
}

HitResult HitWithWeapon(EntityManager& entityManager, unsigned int targetID, float damage, float poiseDamage, unsigned int attackerID,
	DamageKind kind = DamageKind::Weapon, float knockback = 0.0f);
void      FeelTheHit(EntityManager& entityManager, HitResult result, const HitboxComponent& hitbox, const CharacterActionsComponent& owner,
	unsigned int ownerID, unsigned int targetID, const glm::vec3& contactPoint);

// Where the weapon touches the target: the point of the weapon's box nearest to the target, pulled into the target's box
glm::vec3 GetContactPoint(const OBB& weapon, const OBB& target)
{
	const glm::vec3 inWeapon = glm::clamp(glm::transpose(weapon.orientation) * (target.center - weapon.center), -weapon.halfExtents, weapon.halfExtents);
	const glm::vec3 onWeapon = weapon.center + weapon.orientation * inWeapon;
	const glm::vec3 inTarget = glm::clamp(glm::transpose(target.orientation) * (onWeapon - target.center), -target.halfExtents, target.halfExtents);

	return target.center + target.orientation * inTarget;
}

void HitTargetsInReach(EntityManager& entityManager, HitboxComponent& hitbox, unsigned int weaponEntityID,
	unsigned int ownerID, const CharacterActionsComponent& owner)
{
	const OBB   box         = GetWorldBox(entityManager, weaponEntityID, hitbox);
	const float damage      = hitbox.damage      * owner.GetDamageMultiplier() * LevelUpSystem::GetDamageMultiplier(entityManager, ownerID);
	const float poiseDamage = hitbox.poiseDamage * owner.GetDamageMultiplier();

	for (HealthComponent& health : entityManager.GetAllComponents<HealthComponent>())
	{
		const unsigned int targetID = entityManager.GetEntityIDForComponent(health);
		const BoxColliderComponent* targetBody = entityManager.GetComponent<BoxColliderComponent>(targetID);

		if (targetID == ownerID || health.isDead || !targetBody || WasHitThisSwing(hitbox, targetID))
			continue;

		if (!CanHurt(hitbox, *targetBody))
			continue;

		CombatLog::TrackSwing(entityManager, ownerID, owner.attackNumber, targetID, weaponEntityID, box, targetBody->worldOBB);

		if (!Overlaps(box, targetBody->worldOBB))
			continue;

		hitbox.entitiesHitThisSwing.push_back(targetID);

		if (!TryClaimHit(ownerID, owner.attackNumber, targetID))
			continue;

		const HitResult result = HitWithWeapon(entityManager, targetID, damage, poiseDamage, ownerID, DamageKind::Weapon, hitbox.knockback);
		if (result == HitResult::Blocked || result == HitResult::Hurt || result == HitResult::Riposte)
			hitbox.hitFlashSecondsLeft = HitFlashSeconds;

		const glm::vec3 contactPoint = GetContactPoint(box, targetBody->worldOBB);
		if (CombatLog::IsOn())
		{
			const char* names[] = { "missed", "parried", "blocked", "hurt", "riposte" };
			CombatLog::RecordHit(entityManager, ownerID, owner.attackNumber, targetID, names[(int)result], damage, contactPoint, targetBody->worldOBB);
		}

		FeelTheHit(entityManager, result, hitbox, owner, ownerID, targetID, contactPoint);
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
	return health && !health->isDead && !health->isInvulnerable && !IsInvulnerable(entityManager, entityID);
}

void Die(EntityManager& entityManager, unsigned int entityID, HealthComponent& health, unsigned int attackerEntityID)
{
	CombatLog::Event("death," + GetEntityName(entityManager, entityID));

	health.isDead = true;

	if (health.deathSlowMotionSeconds > 0.0f)
		TimeScaleSystem::SlowMotion(health.deathSlowMotionSeconds, health.deathSlowMotionScale);

	// Nobody keeps the lock on a dead body
	if (LockOnTargetComponent* lockOnTarget = entityManager.GetComponent<LockOnTargetComponent>(entityID))
		lockOnTarget->isLockable = false;

	CharacterActionsSystem::PlayDeath(entityManager, entityID);
	CharacterSoundsSystem::Play(entityManager, entityID, &CharacterSoundsComponent::deathSound);
	CharacterSoundsSystem::PlayVoice(entityManager, entityID, &CharacterSoundsComponent::deathVoice);
	CurrencySystem::OnDeath(entityManager, entityID, attackerEntityID);
}

std::vector<DamageEvent>& GetDamageEvents()
{
	static std::vector<DamageEvent> events;
	return events;
}

// Takes the damage away from the health (as much of it as the character lets through from that kind of blow).
// True when the character is still alive
bool IsPlayerCharacter(EntityManager& entityManager, unsigned int entityID);

bool LoseHealth(EntityManager& entityManager, unsigned int entityID, float damage, unsigned int attackerEntityID, DamageKind kind)
{
	HealthComponent& health = *entityManager.GetComponent<HealthComponent>(entityID);
	damage *= glm::max(health.GetDamageTaken(kind), 0.0f);
	GetDamageEvents().push_back({ entityID, attackerEntityID, damage, kind });

	// A script may keep the character above a floor (a boss that changes its phase there instead of dying),
	// and the developer's god mode keeps the player alive (see DeveloperCheats)
	float minimum = health.minimumHealth;
	if (IsPlayerCharacter(entityManager, entityID))
		minimum = glm::max(minimum, DeveloperCheats::GetPlayerHealthFloor(health.maxHealth));
	const float floor = glm::clamp(minimum, 0.0f, health.currentHealth);
	health.currentHealth = glm::max(health.currentHealth - damage, floor);
	CharacterSoundsSystem::Play(entityManager, entityID, &CharacterSoundsComponent::hurtSound);
	if (health.currentHealth > 0.0f && damage > 0.0f)
	{
		const CharacterSoundsComponent* sounds = entityManager.GetComponent<CharacterSoundsComponent>(entityID);
		CharacterSoundsSystem::PlayVoice(entityManager, entityID, &CharacterSoundsComponent::painVoice, sounds ? sounds->painVoiceChance : 0.0f);
	}

	SUNTA_ENGINE_LOG_INFO("Combat: '{0}' hit '{1}' for {2} ({3} health left)", GetEntityName(entityManager, attackerEntityID),
		GetEntityName(entityManager, entityID), damage, health.currentHealth);

	if (health.currentHealth > 0.0f)
		return true;

	Die(entityManager, entityID, health, attackerEntityID);
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

	// Just staggered: it shrugs blows off for a moment
	if (poise->secondsSinceStagger < poise->staggerImmunitySeconds)
		return false;

	poise->currentPoise   -= poiseDamage;

	if (poise->currentPoise > 0.0f)
		return false;

	poise->currentPoise        = poise->maxPoise;
	poise->secondsSinceStagger = 0.0f;
	return true;
}

void RecoverPoise(PoiseComponent& poise, float deltaTime)
{
	poise.secondsSinceHit     += deltaTime;
	poise.secondsSinceStagger += deltaTime;

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
	CharacterSoundsSystem::Play(entityManager, defenderID, &CharacterSoundsComponent::parrySound);

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

	// The blow lands on the shield either way, held or broken through
	CharacterSoundsSystem::Play(entityManager, defenderID, &CharacterSoundsComponent::blockSound);

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

// Goes through the target's defences: a parry turns it away (not a spell), a shield takes it, poise keeps the target
// on its feet
HitResult HitWithWeapon(EntityManager& entityManager, unsigned int targetID, float damage, float poiseDamage, unsigned int attackerID,
	DamageKind kind, float knockback)
{
	if (!CanBeHurt(entityManager, targetID))
		return HitResult::Missed;

	if (kind == DamageKind::Weapon && TryParry(entityManager, targetID, attackerID))
		return HitResult::Parried;

	// One projectile of a volley has just hurt it: the rest of the volley passes
	HealthComponent* targetHealth = entityManager.GetComponent<HealthComponent>(targetID);
	if (kind == DamageKind::Spell && targetHealth)
	{
		if (targetHealth->secondsSinceSpellHit < targetHealth->spellImmunitySeconds)
			return HitResult::Missed;
		targetHealth->secondsSinceSpellHit = 0.0f;
	}

	const BlockResult block = TryBlock(entityManager, targetID, attackerID, damage);

	if (block == BlockResult::Blocked)
	{
		if (damage <= 0.0f || LoseHealth(entityManager, targetID, damage, attackerID, kind))
			CharacterActionsSystem::PlayBlockImpact(entityManager, targetID);
		return HitResult::Blocked;
	}

	const float riposteMultiplier = GetRiposteMultiplier(entityManager, targetID, attackerID);
	damage *= riposteMultiplier;

	const CharacterActionsComponent* targetActions = entityManager.GetComponent<CharacterActionsComponent>(targetID);
	const bool hasHyperArmor = targetActions && targetActions->HasHyperArmor();
	const bool staggers = block == BlockResult::GuardBroken || (!hasHyperArmor && BreaksPoise(entityManager, targetID, poiseDamage));

	// A heavy blow (a heavy attack, a leap) staggers with the heavy hit reaction, when the target has one
	const CharacterActionsComponent* attackerActions = entityManager.GetComponent<CharacterActionsComponent>(attackerID);
	const bool isHeavyBlow = attackerActions && (attackerActions->currentAction == CharacterAction::HeavyAttack
	                                             || attackerActions->currentAction == CharacterAction::LeapAttack);

	if (LoseHealth(entityManager, targetID, damage, attackerID, kind) && staggers)
	{
		// A blow that throws: away from the attacker, along the ground
		const TransformComponent* target   = entityManager.GetComponent<TransformComponent>(targetID);
		const TransformComponent* attacker = entityManager.GetComponent<TransformComponent>(attackerID);
		const bool isThrown = knockback > 0.0f && target && attacker
			&& CharacterActionsSystem::PlayKnockback(entityManager, targetID, target->position - attacker->position, knockback);
		if (!isThrown)
			CharacterActionsSystem::PlayHitReaction(entityManager, targetID, isHeavyBlow);
	}

	if (block == BlockResult::GuardBroken)
		return HitResult::Blocked;

	return riposteMultiplier > 1.0f ? HitResult::Riposte : HitResult::Hurt;
}

// ========== How a hit feels ==========

bool IsPlayerCharacter(EntityManager& entityManager, unsigned int entityID)
{
	const CharacterControllerComponent* controller = entityManager.GetComponent<CharacterControllerComponent>(entityID);
	return controller && controller->isPlayerControlled;
}

void BurstShieldEffect(EntityManager& entityManager, unsigned int defenderID, const glm::vec3& point, int count, const glm::vec3& direction)
{
	const ShieldComponent* shield = FindShield(entityManager, defenderID);
	if (shield && !shield->blockEffect.empty())
		ParticleSystem::BurstAt(entityManager, shield->blockEffect, point, count, direction);
}

void BurstBodyEffect(EntityManager& entityManager, unsigned int targetID, const glm::vec3& point, int count, const glm::vec3& direction)
{
	const HealthComponent* health = entityManager.GetComponent<HealthComponent>(targetID);
	if (health && !health->hitEffect.empty())
		ParticleSystem::BurstAt(entityManager, health->hitEffect, point, count, direction);
}

// The way the blow went: from the attacker to the one hit, along the ground (the effects face it: a slash, a spray)
glm::vec3 BlowDirection(EntityManager& entityManager, unsigned int attackerID, unsigned int targetID)
{
	const WorldMatrixComponent* attacker = entityManager.GetComponent<WorldMatrixComponent>(attackerID);
	const WorldMatrixComponent* target   = entityManager.GetComponent<WorldMatrixComponent>(targetID);
	if (!attacker || !target)
		return glm::vec3(0.0f);

	glm::vec3 direction = glm::vec3(target->matrix[3]) - glm::vec3(attacker->matrix[3]);
	direction.y = 0.0f;
	return glm::length(direction) > 0.0001f ? glm::normalize(direction) : glm::vec3(0.0f);
}

// Sparks or blood where the weapon landed; a freeze and a shake when the player gives or takes the blow
// (two enemies trading hits far away don't stop the game)
void FeelTheHit(EntityManager& entityManager, HitResult result, const HitboxComponent& hitbox, const CharacterActionsComponent& owner,
	unsigned int ownerID, unsigned int targetID, const glm::vec3& contactPoint)
{
	const float heaviness = owner.GetDamageMultiplier() > 1.0f ? HeavyFeelMultiplier : 1.0f;
	float hitStopSeconds  = hitbox.hitStopSeconds * heaviness;
	float cameraShake     = hitbox.cameraShake    * heaviness;
	const glm::vec3 direction = BlowDirection(entityManager, ownerID, targetID);

	switch (result)
	{
	case HitResult::Missed:
	{
		// Only a body a script protects shows the blow glancing off (a roll's invulnerability is a dodge: nothing touched)
		const HealthComponent* health = entityManager.GetComponent<HealthComponent>(targetID);
		if (!health || !health->isInvulnerable || health->isDead || health->deflectEffect.empty())
			return;
		ParticleSystem::BurstAt(entityManager, health->deflectEffect, contactPoint, DeflectEffectBurst, direction);
		hitStopSeconds *= DeflectedFeelMultiplier;
		cameraShake    *= DeflectedFeelMultiplier;
		break;
	}
	case HitResult::Parried:
		BurstShieldEffect(entityManager, targetID, contactPoint, ParryEffectBurst, direction);
		hitStopSeconds = ParryHitStopSeconds;
		cameraShake    = ParryCameraShake;
		break;
	case HitResult::Blocked:
		BurstShieldEffect(entityManager, targetID, contactPoint, BlockEffectBurst, direction);
		hitStopSeconds *= BlockedFeelMultiplier;
		cameraShake    *= BlockedFeelMultiplier;
		break;
	case HitResult::Hurt:
		BurstBodyEffect(entityManager, targetID, contactPoint, (int)(HitEffectBurst * heaviness), direction);
		break;
	case HitResult::Riposte:
		BurstBodyEffect(entityManager, targetID, contactPoint, HitEffectBurst * 3, direction);
		hitStopSeconds = RiposteHitStopSeconds;
		cameraShake    = RiposteCameraShake;
		break;
	}

	if (!IsPlayerCharacter(entityManager, ownerID) && !IsPlayerCharacter(entityManager, targetID))
		return;

	TimeScaleSystem::HitStop(hitStopSeconds);
	FollowCameraSystem::Shake(entityManager, cameraShake);
	PostProcessing::KickFromHit(cameraShake);
}

}

void CombatSystem::PrepareForPlay(EntityManager& entityManager)
{
	GetAttackHits().clear();
	GetDamageEvents().clear();

	for (HealthComponent& health : entityManager.GetAllComponents<HealthComponent>())
	{
		health.currentHealth  = health.maxHealth;
		health.isDead         = false;
		health.isInvulnerable = false;
		health.minimumHealth  = 0.0f;
	}

	for (PoiseComponent& poise : entityManager.GetAllComponents<PoiseComponent>())
	{
		poise.currentPoise        = poise.maxPoise;
		poise.secondsSinceHit     = 0.0f;
		poise.secondsSinceStagger = 1000.0f;
	}
}

namespace
{

// How many specks the shockwave's effect throws at once
constexpr int ShockwaveBurst = 140;

// Only targets within this height of the shockwave are hurt (not someone standing on a wall above)
constexpr float ShockwaveHeight = 2.0f;

// The camera feels a shockwave up to this many of its radii away
constexpr float ShockwaveShakeReach = 3.0f;

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
	if ((actions.shockwaveRadius <= 0.0f && actions.jumpPlungeShockwaveRadius <= 0.0f) || actions.shockwaveAttackNumber == actions.attackNumber)
		return;

	const bool isHeavy = actions.currentAction == CharacterAction::HeavyAttack && actions.actionProgress >= actions.heavyHitFrom;
	const bool isLeap  = actions.currentAction == CharacterAction::LeapAttack  && actions.actionProgress >= actions.leapHitFrom;
	// The plunge from a jump goes off where it lands, under the character
	const bool isPlunge = actions.currentAction == CharacterAction::JumpAttack && actions.isPlunging && actions.hasPlungeLanded;
	if ((!isHeavy && !isLeap && !isPlunge) || (!isPlunge && actions.shockwaveRadius <= 0.0f)
	    || (isPlunge && actions.jumpPlungeShockwaveRadius <= 0.0f))
		return;
	const float radius = isPlunge ? actions.jumpPlungeShockwaveRadius : actions.shockwaveRadius;
	const float damage = isPlunge ? actions.jumpPlungeShockwaveDamage : actions.shockwaveDamage;

	actions.shockwaveAttackNumber = actions.attackNumber;

	const TransformComponent*           transform  = entityManager.GetComponent<TransformComponent>(ownerID);
	const CharacterControllerComponent* controller = entityManager.GetComponent<CharacterControllerComponent>(ownerID);
	if (!transform || !controller)
		return;

	const glm::vec3 forward = GameplayMath::GetDirectionFromYawAndPitch(transform->rotation.y - controller->facingOffsetDegrees, 0.0f);
	const glm::vec3 center  = transform->position + forward * (isPlunge ? actions.shockwaveDistance * 0.3f : actions.shockwaveDistance);
	const bool      isPlayerSide = IsOnPlayersSide(entityManager, ownerID);

	BurstEffect(entityManager, ownerID, actions.shockwaveEffect);
	if (isPlunge && !actions.jumpPlungeEffect.empty())
		EffectSystem::Play(entityManager, actions.jumpPlungeEffect, center, forward);
	CharacterSoundsSystem::Play(entityManager, ownerID, &CharacterSoundsComponent::shockwaveSound);
	FollowCameraSystem::ShakeFrom(entityManager, actions.shockwaveCameraShake, center, radius * ShockwaveShakeReach);

	for (HealthComponent& health : entityManager.GetAllComponents<HealthComponent>())
	{
		const unsigned int targetID = entityManager.GetEntityIDForComponent(health);
		const TransformComponent* target = entityManager.GetComponent<TransformComponent>(targetID);
		if (targetID == ownerID || !target || IsOnPlayersSide(entityManager, targetID) == isPlayerSide)
			continue;

		const glm::vec3 offset = target->position - center;
		if (glm::length(glm::vec2(offset.x, offset.z)) <= radius && std::abs(offset.y) <= ShockwaveHeight)
			CombatSystem::ApplyDamage(entityManager, targetID, damage, ownerID);
	}
}

// The moment an attack starts to hit, once per attack: its swing effect in front of the character
void UpdateSwingEffect(EntityManager& entityManager, unsigned int ownerID, CharacterActionsComponent& actions)
{
	if ((actions.swingEffect.empty() && actions.heavySwingEffect.empty()) || actions.swingEffectAttackNumber == actions.attackNumber
		|| !actions.IsAttacking()
		|| actions.actionProgress < glm::mix(actions.GetHitFrom(), actions.GetHitTo(), actions.swingEffectAt))
		return;

	actions.swingEffectAttackNumber = actions.attackNumber;

	const bool isHeavy = actions.currentAction == CharacterAction::HeavyAttack || actions.currentAction == CharacterAction::LeapAttack
	                     || (actions.currentAction == CharacterAction::JumpAttack && actions.isPlunging);
	const std::string& effect = isHeavy && !actions.heavySwingEffect.empty() ? actions.heavySwingEffect : actions.swingEffect;

	const TransformComponent*           transform  = entityManager.GetComponent<TransformComponent>(ownerID);
	const CharacterControllerComponent* controller = entityManager.GetComponent<CharacterControllerComponent>(ownerID);
	if (effect.empty() || !transform || !controller)
		return;

	const glm::vec3 forward = GameplayMath::GetDirectionFromYawAndPitch(transform->rotation.y - controller->facingOffsetDegrees, 0.0f);
	const glm::vec3 point   = transform->position + glm::vec3(0.0f, actions.swingEffectHeight, 0.0f) + forward * actions.swingEffectDistance;
	EffectSystem::Play(entityManager, effect, point, forward);
}

}

void CombatSystem::Update(EntityManager& entityManager, float deltaTime)
{
	CombatLog::Advance(deltaTime);

	for (CharacterActionsComponent& actions : entityManager.GetAllComponents<CharacterActionsComponent>())
	{
		UpdateShockwave(entityManager, entityManager.GetEntityIDForComponent(actions), actions);
		UpdateSwingEffect(entityManager, entityManager.GetEntityIDForComponent(actions), actions);
	}

	for (PoiseComponent& poise : entityManager.GetAllComponents<PoiseComponent>())
		RecoverPoise(poise, deltaTime);

	for (HealthComponent& health : entityManager.GetAllComponents<HealthComponent>())
		health.secondsSinceSpellHit += deltaTime;

	for (HitboxComponent& hitbox : entityManager.GetAllComponents<HitboxComponent>())
	{
		hitbox.hitFlashSecondsLeft = glm::max(hitbox.hitFlashSecondsLeft - deltaTime, 0.0f);

		const unsigned int weaponEntityID = entityManager.GetEntityIDForComponent(hitbox);

		unsigned int ownerID = 0;
		if (CombatLog::IsTracing() && TryFindWeaponOwner(entityManager, weaponEntityID, ownerID))
		{
			const CharacterActionsComponent* attacker = entityManager.GetComponent<CharacterActionsComponent>(ownerID);
			if (attacker && attacker->IsAttacking())
				CombatLog::Trace(entityManager, ownerID, weaponEntityID, GetWorldBox(entityManager, weaponEntityID, hitbox),
					attacker->actionProgress, attacker->IsInHitWindow());
		}

		const CharacterActionsComponent* owner = GetStrikingOwner(entityManager, weaponEntityID, ownerID);
		if (!owner || !hitbox.isEnabled)
			continue;

		StartNewSwingIfNeeded(hitbox, *owner);
		HitTargetsInReach(entityManager, hitbox, weaponEntityID, ownerID, *owner);
	}
}

bool CombatSystem::ApplyDamage(EntityManager& entityManager, unsigned int targetEntityID, float damage, unsigned int attackerEntityID)
{
	if (!CanBeHurt(entityManager, targetEntityID))
		return false;

	const CharacterActionsComponent* targetActions = entityManager.GetComponent<CharacterActionsComponent>(targetEntityID);
	const bool hasHyperArmor = targetActions && targetActions->HasHyperArmor();

	if (LoseHealth(entityManager, targetEntityID, damage, attackerEntityID, DamageKind::Other) && !hasHyperArmor)
		CharacterActionsSystem::PlayHitReaction(entityManager, targetEntityID);

	return true;
}

std::vector<DamageEvent> CombatSystem::TakeDamageEvents()
{
	std::vector<DamageEvent> events;
	events.swap(GetDamageEvents());
	return events;
}

bool CombatSystem::HitWithSpell(EntityManager& entityManager, unsigned int targetEntityID, float damage, float poiseDamage,
	unsigned int casterEntityID, const glm::vec3& point, float hitStopSeconds, float cameraShake)
{
	const HitResult result = HitWithWeapon(entityManager, targetEntityID, damage, poiseDamage, casterEntityID, DamageKind::Spell);

	switch (result)
	{
	case HitResult::Missed:
		return false;
	case HitResult::Blocked:
		BurstShieldEffect(entityManager, targetEntityID, point, BlockEffectBurst, BlowDirection(entityManager, casterEntityID, targetEntityID));
		hitStopSeconds *= BlockedFeelMultiplier;
		cameraShake    *= BlockedFeelMultiplier;
		break;
	default:
		BurstBodyEffect(entityManager, targetEntityID, point, HitEffectBurst, BlowDirection(entityManager, casterEntityID, targetEntityID));
		break;
	}

	if (IsPlayerCharacter(entityManager, casterEntityID) || IsPlayerCharacter(entityManager, targetEntityID))
	{
		TimeScaleSystem::HitStop(hitStopSeconds);
		FollowCameraSystem::Shake(entityManager, cameraShake);
		PostProcessing::KickFromHit(cameraShake);
	}
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
