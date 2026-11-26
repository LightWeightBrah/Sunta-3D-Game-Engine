#include "Core/SuntaPreCompiled.h"
#include "CombatSystem.h"

#include <random>
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
// A deflect is the beat of a Sekiro duel: sparks and a clang on every one, a freeze short enough to keep the rhythm
constexpr int   DeflectSparksBurst    = 40;
constexpr float DeflectHitStopSeconds = 0.07f;
constexpr float DeflectCameraShake    = 0.15f;
constexpr float RiposteCameraShake    = 0.5f;
// How far (a part of its distance) and how long the camera comes in for the player's finisher
constexpr float FinisherPunchIn        = 0.4f;
constexpr float FinisherPunchInSeconds = 1.1f;

// What a weapon's hit came to
enum class HitResult
{
	Missed,   // the target couldn't be hurt (dead, rolling)
	Parried,  // turned away by a parry: nobody is hurt, the attacker staggers
	Deflected,// turned away by a block raised just in time: nobody is hurt, the attacker's posture fills
	Blocked,  // the shield took it (or the guard broke)
	Hurt,     // it landed on the body
	Riposte   // it landed on a parried attacker: a critical blow
};

// The attacker's blow hurt: it wins back a part of its own lost health (see HealthComponent: the rally)
void Rally(EntityManager& entityManager, unsigned int attackerID, float damage);

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
		if (result == HitResult::Hurt || result == HitResult::Riposte)
			Rally(entityManager, ownerID, damage);

		const glm::vec3 contactPoint = GetContactPoint(box, targetBody->worldOBB);
		if (CombatLog::IsOn())
		{
			const char* names[] = { "missed", "parried", "deflected", "blocked", "hurt", "riposte" };
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
	const float before = health.currentHealth;
	health.currentHealth = glm::max(health.currentHealth - damage, floor);

	// The rally: what this blow took can be won back for a while
	if (health.rallyWindowSeconds > 0.0f)
	{
		health.rallyHealth      = glm::min(health.rallyHealth + (before - health.currentHealth), health.maxHealth - health.currentHealth);
		health.rallySecondsLeft = health.rallyWindowSeconds;
	}
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

// ========== The rally (Bloodborne) ==========

// After the window the part still to be won back drains this fast (a part of the full health a second)
constexpr float RallyDrainPerSecond = 0.25f;

// The attacker's blow hurt for 'damage': it wins back a part of its own lost health
void Rally(EntityManager& entityManager, unsigned int attackerID, float damage)
{
	HealthComponent* health = entityManager.GetComponent<HealthComponent>(attackerID);
	if (!health || health->isDead || health->rallyHealth <= 0.0f || damage <= 0.0f)
		return;

	const float gain = glm::min(health->rallyHealth, damage * health->rallyPerDamage);
	health->currentHealth = glm::min(health->currentHealth + gain, health->maxHealth);
	health->rallyHealth  -= gain;
}

void UpdateRally(HealthComponent& health, float deltaTime)
{
	if (health.rallyHealth <= 0.0f)
		return;
	if (health.isDead)
	{
		health.rallyHealth = 0.0f;
		return;
	}
	health.rallySecondsLeft -= deltaTime;
	if (health.rallySecondsLeft < 0.0f)
		health.rallyHealth = glm::max(health.rallyHealth - health.maxHealth * RallyDrainPerSecond * deltaTime, 0.0f);
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

// ========== Posture (Sekiro) ==========

// The posture breaks: the character reels (the long stagger of a parried attack) and is open to a finisher
void BreakPosture(EntityManager& entityManager, unsigned int entityID, PostureComponent& posture)
{
	posture.currentPosture = posture.maxPosture;
	posture.brokenLeft     = glm::max(posture.brokenSeconds, 0.01f);
	posture.breaks++;

	CharacterActionsSystem::PlayParried(entityManager, entityID);

	if (const TransformComponent* transform = entityManager.GetComponent<TransformComponent>(entityID); transform && !posture.breakEffect.empty())
		ParticleSystem::BurstAt(entityManager, posture.breakEffect, transform->position + glm::vec3(0.0f, 1.4f, 0.0f), ParryEffectBurst);

	SUNTA_ENGINE_LOG_INFO("Combat: the posture of '{0}' broke", GetEntityName(entityManager, entityID));
}

// Fills the posture of a character that has one; true when this broke it
bool FillPosture(EntityManager& entityManager, unsigned int entityID, float amount)
{
	PostureComponent* posture = entityManager.GetComponent<PostureComponent>(entityID);
	if (!posture || amount <= 0.0f || posture->IsBroken())
		return false;

	const HealthComponent* health = entityManager.GetComponent<HealthComponent>(entityID);
	if (health && health->isDead)
		return false;

	posture->currentPosture   = glm::min(posture->currentPosture + amount, posture->maxPosture);
	posture->secondsSinceFill = 0.0f;

	if (posture->currentPosture < posture->maxPosture)
		return false;

	BreakPosture(entityManager, entityID, *posture);
	return true;
}

void RecoverPosture(EntityManager& entityManager, PostureComponent& posture, float deltaTime)
{
	if (posture.brokenLeft > 0.0f)
	{
		// The moment for the finisher passed: the guard comes back, the posture starts from the middle
		posture.brokenLeft -= deltaTime;
		if (posture.brokenLeft <= 0.0f)
		{
			posture.brokenLeft       = 0.0f;
			posture.currentPosture   = posture.maxPosture * 0.5f;
			posture.secondsSinceFill = 0.0f;
		}
		return;
	}

	posture.secondsSinceFill += deltaTime;
	if (posture.secondsSinceFill < posture.recoverDelaySeconds)
		return;

	// A hurt character keeps its posture longer (as in Sekiro)
	float healthFactor = 1.0f;
	if (const HealthComponent* health = entityManager.GetComponent<HealthComponent>(entityManager.GetEntityIDForComponent(posture));
	    health && health->maxHealth > 0.0f)
	{
		const float healthFraction = glm::clamp(health->currentHealth / health->maxHealth, 0.0f, 1.0f);
		healthFactor = glm::mix(posture.lowHealthRecoverFactor, 1.0f, healthFraction);
	}

	posture.currentPosture = glm::max(posture.currentPosture - posture.recoverPerSecond * healthFactor * deltaTime, 0.0f);
}

// A blow on a character whose posture is broken is the finisher: a part of its full health in one blow
bool TryFinisher(EntityManager& entityManager, unsigned int targetID, unsigned int attackerID, float& inOutDamage)
{
	PostureComponent* posture = entityManager.GetComponent<PostureComponent>(targetID);
	const HealthComponent* health = entityManager.GetComponent<HealthComponent>(targetID);
	if (!posture || !posture->IsBroken() || !health)
		return false;

	inOutDamage = glm::max(inOutDamage, health->maxHealth * posture->finisherDamage);
	posture->brokenLeft       = 0.0f;
	posture->currentPosture   = 0.0f;
	posture->secondsSinceFill = 0.0f;
	posture->finishers++;

	if (posture->finisherSlowMotion > 0.0f && posture->finisherSlowSeconds > 0.0f)
		TimeScaleSystem::SlowMotion(posture->finisherSlowSeconds, posture->finisherSlowMotion);

	// The player's finisher is seen closer: the camera glides in for the blow and back out
	if (IsPlayerCharacter(entityManager, attackerID))
		FollowCameraSystem::PunchIn(entityManager, FinisherPunchIn, FinisherPunchInSeconds);

	if (const TransformComponent* transform = entityManager.GetComponent<TransformComponent>(targetID); transform && !posture->finisherEffect.empty())
		ParticleSystem::BurstAt(entityManager, posture->finisherEffect, transform->position + glm::vec3(0.0f, 1.3f, 0.0f), ParryEffectBurst * 2);

	SUNTA_ENGINE_LOG_INFO("Combat: '{0}' landed a finisher on '{1}'", GetEntityName(entityManager, attackerID), GetEntityName(entityManager, targetID));
	return true;
}

// The blow playing now can not be blocked or parried (the perilous attacks of Sekiro): only a dodge gets out of it
bool IsPerilousBlow(EntityManager& entityManager, unsigned int attackerID)
{
	const CharacterActionsComponent* attacker = entityManager.GetComponent<CharacterActionsComponent>(attackerID);
	return attacker && attacker->IsPerilousAttack();
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

// The blow lands just after the defender raised the block, from the front: deflected (Sekiro)
bool TryDeflect(EntityManager& entityManager, unsigned int defenderID, unsigned int attackerID);

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

bool TryDeflect(EntityManager& entityManager, unsigned int defenderID, unsigned int attackerID)
{
	const CharacterActionsComponent* defender = GetActions(entityManager, defenderID);
	const ShieldComponent*           shield   = FindShield(entityManager, defenderID);
	if (!defender || !defender->IsDeflecting() || !shield || !IsInFrontOfShield(entityManager, defenderID, attackerID, *shield))
		return false;

	// No block impact: a deflect leaves the defender free at once (to deflect the next blow, or to strike the opening)
	CharacterSoundsSystem::Play(entityManager, defenderID, &CharacterSoundsComponent::parrySound);
	if (const PostureComponent* attackerPosture = entityManager.GetComponent<PostureComponent>(attackerID))
		FillPosture(entityManager, attackerID, attackerPosture->fromBeingParried);

	SUNTA_ENGINE_LOG_INFO("Combat: '{0}' deflected '{1}'", GetEntityName(entityManager, defenderID), GetEntityName(entityManager, attackerID));
	return true;
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

	const bool isPerilous = kind == DamageKind::Weapon && IsPerilousBlow(entityManager, attackerID);

	if (kind == DamageKind::Weapon && !isPerilous && TryParry(entityManager, targetID, attackerID))
	{
		// The deflect: the posture of the parried attacker takes it
		if (const PostureComponent* attackerPosture = entityManager.GetComponent<PostureComponent>(attackerID))
			FillPosture(entityManager, attackerID, attackerPosture->fromBeingParried);
		return HitResult::Parried;
	}

	if (kind == DamageKind::Weapon && !isPerilous && TryDeflect(entityManager, targetID, attackerID))
		return HitResult::Deflected;

	// One projectile of a volley has just hurt it: the rest of the volley passes
	HealthComponent* targetHealth = entityManager.GetComponent<HealthComponent>(targetID);
	if (kind == DamageKind::Spell && targetHealth)
	{
		if (targetHealth->secondsSinceSpellHit < targetHealth->spellImmunitySeconds)
			return HitResult::Missed;
		targetHealth->secondsSinceSpellHit = 0.0f;
	}

	const float damageBeforeShield = damage;
	const BlockResult block = isPerilous ? BlockResult::NotBlocked : TryBlock(entityManager, targetID, attackerID, damage);

	if (block == BlockResult::Blocked)
	{
		// Blocking wears the guard down: too many blocks in a row break the posture
		if (const PostureComponent* targetPosture = entityManager.GetComponent<PostureComponent>(targetID);
		    targetPosture && FillPosture(entityManager, targetID, damageBeforeShield * targetPosture->fromBlocks))
			return HitResult::Blocked;
		if (damage <= 0.0f || LoseHealth(entityManager, targetID, damage, attackerID, kind))
			CharacterActionsSystem::PlayBlockImpact(entityManager, targetID);
		return HitResult::Blocked;
	}

	const bool isFinisher = kind == DamageKind::Weapon && TryFinisher(entityManager, targetID, attackerID, damage);
	const float riposteMultiplier = isFinisher ? 1.0f : GetRiposteMultiplier(entityManager, targetID, attackerID);
	damage *= riposteMultiplier;

	// The blow fills the posture too (a broken one was just finished)
	bool breaksPosture = false;
	if (const PostureComponent* targetPosture = entityManager.GetComponent<PostureComponent>(targetID); targetPosture && !isFinisher)
		breaksPosture = FillPosture(entityManager, targetID, poiseDamage * targetPosture->fromHits);

	const CharacterActionsComponent* targetActions = entityManager.GetComponent<CharacterActionsComponent>(targetID);
	const bool hasHyperArmor = targetActions && targetActions->HasHyperArmor();
	const bool staggers = block == BlockResult::GuardBroken || (!hasHyperArmor && BreaksPoise(entityManager, targetID, poiseDamage));

	// A heavy blow (a heavy attack, a leap) staggers with the heavy hit reaction, when the target has one
	const CharacterActionsComponent* attackerActions = entityManager.GetComponent<CharacterActionsComponent>(attackerID);
	const bool isHeavyBlow = attackerActions && (attackerActions->currentAction == CharacterAction::HeavyAttack
	                                             || attackerActions->currentAction == CharacterAction::LeapAttack);

	// The attacker's own throwing blows (its combo's last blow, its heavy attacks): thrown even without a stagger
	float ownKnockback = 0.0f;
	if (attackerActions && !hasHyperArmor && block != BlockResult::GuardBroken)
	{
		const int lastLightStep = 2 + static_cast<int>(attackerActions->extraLightAttackAnimations.size());
		if (attackerActions->currentAction == CharacterAction::LightAttack && attackerActions->lightAttackStep >= lastLightStep)
			ownKnockback = attackerActions->finisherKnockback;
		else if (isHeavyBlow)
		{
			static std::mt19937 throwRandom(std::random_device{}());
			if (std::uniform_real_distribution<float>(0.0f, 1.0f)(throwRandom) < attackerActions->heavyKnockbackChance)
				ownKnockback = attackerActions->heavyKnockback;
		}
	}
	knockback = glm::max(knockback, ownKnockback);

	if (LoseHealth(entityManager, targetID, damage, attackerID, kind) && !breaksPosture && (staggers || ownKnockback > 0.0f))
	{
		// A blow that throws: away from the attacker, along the ground
		const TransformComponent* target   = entityManager.GetComponent<TransformComponent>(targetID);
		const TransformComponent* attacker = entityManager.GetComponent<TransformComponent>(attackerID);
		const bool isThrown = knockback > 0.0f && target && attacker
			&& CharacterActionsSystem::PlayKnockback(entityManager, targetID, target->position - attacker->position, knockback,
				isHeavyBlow);
		if (!isThrown)
			CharacterActionsSystem::PlayHitReaction(entityManager, targetID, isHeavyBlow);
	}

	if (block == BlockResult::GuardBroken)
		return HitResult::Blocked;

	return (isFinisher || riposteMultiplier > 1.0f) ? HitResult::Riposte : HitResult::Hurt;
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
	case HitResult::Deflected:
		// Between a block and a parry: a bright burst of sparks, a short freeze
		BurstShieldEffect(entityManager, targetID, contactPoint, DeflectSparksBurst, direction);
		hitStopSeconds = DeflectHitStopSeconds;
		cameraShake    = DeflectCameraShake;
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

	for (PostureComponent& posture : entityManager.GetAllComponents<PostureComponent>())
	{
		posture.currentPosture   = 0.0f;
		posture.secondsSinceFill = 1000.0f;
		posture.brokenLeft       = 0.0f;
		posture.breaks           = 0;
		posture.finishers        = 0;
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

	for (PostureComponent& posture : entityManager.GetAllComponents<PostureComponent>())
		RecoverPosture(entityManager, posture, deltaTime);

	for (HealthComponent& health : entityManager.GetAllComponents<HealthComponent>())
	{
		health.secondsSinceSpellHit += deltaTime;
		UpdateRally(health, deltaTime);
	}

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

bool CombatSystem::FillPosture(EntityManager& entityManager, unsigned int entityID, float amount)
{
	return Sunta::FillPosture(entityManager, entityID, amount);
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
