#include "Core/SuntaPreCompiled.h"
#include "BossSystem.h"

#include <glm/glm.hpp>

#include "CharacterActionsSystem.h"
#include "Audio/Audio.h"
#include "Audio/AudioSystem.h"
#include "CutsceneSystem.h"
#include "CharacterControllerSystem.h"
#include "GameplayMath.h"
#include "HUDSystem.h"
#include "Core/InputManager.h"
#include "Core/KeyCodes.h"
#include "ECS/Component.h"
#include "ECS/EntityManager.h"
#include "Scene/EntityHierarchy.h"

namespace Sunta
{

namespace
{

// An awake boss hears the player anywhere in its arena, so it never stands waiting while the player walks around it
constexpr float AwakeHearingDistance = 100.0f;

// Phase two: how long the boss takes to grow to its new size, how its glow pulses (part of the glow, times a second),
// and how many specks each of its emitters throws at once when it starts
constexpr float GrowSeconds      = 1.5f;
constexpr float GlowPulse        = 0.25f;
constexpr float GlowPulseSpeed   = 3.0f;
constexpr int   PhaseTwoBurst    = 80;

// Crossing a fog gate: how close to it E works, how long the walk through takes, how far behind it the player comes out
constexpr int         CrossKey           = SUNTA_KEY_E;
constexpr const char* CrossPrompt        = "Traverse the white fog   [E]";
constexpr float       CrossReachDistance = 2.5f;
constexpr float       CrossSeconds       = 1.2f;
constexpr float       CrossBehindFog     = 1.5f;

// A defeated boss goes out like a fire: no new embers, the glow fades away in this time
constexpr float GoingOutSeconds  = 3.0f;

// The music of a fight comes in quickly when the fog closes, dies away slowly after a death and after a victory
constexpr float BattleMusicFadeInSeconds  = 1.5f;
constexpr float BattleMusicFadeOutSeconds = 3.0f;
constexpr float VictoryMusicFadeOutSeconds = 4.0f;

// ========== Small helpers ==========

bool IsInside(const FogGateComponent& fog, const glm::vec3& point)
{
	const glm::vec3 distance = glm::abs(point - fog.arenaCenter);
	return distance.x <= fog.arenaHalfExtents.x && distance.y <= fog.arenaHalfExtents.y && distance.z <= fog.arenaHalfExtents.z;
}

bool TryGetPosition(EntityManager& entityManager, unsigned int entityID, glm::vec3& outPosition)
{
	const TransformComponent* transform = entityManager.GetComponent<TransformComponent>(entityID);
	if (transform)
		outPosition = transform->position;
	return transform != nullptr;
}

bool IsAlive(EntityManager& entityManager, unsigned int entityID)
{
	const HealthComponent* health = entityManager.GetComponent<HealthComponent>(entityID);
	return health && !health->isDead;
}

// The weapon of the boss: its child with a Hitbox (the sword)
HitboxComponent* FindWeapon(EntityManager& entityManager, unsigned int bossID)
{
	for (unsigned int childID : EntityHierarchy::GetChildren(entityManager, bossID))
	{
		if (HitboxComponent* hitbox = entityManager.GetComponent<HitboxComponent>(childID))
			return hitbox;
	}

	return nullptr;
}

// ========== The bosses ==========

// The boss and its children (the weapon in its hand, the effects on its bones)
std::vector<unsigned int> GetBossAndChildren(EntityManager& entityManager, unsigned int bossID)
{
	std::vector<unsigned int> entities = { bossID };
	for (unsigned int childID : EntityHierarchy::GetChildren(entityManager, bossID))
		entities.push_back(childID);

	return entities;
}

void RememberRestingValues(EntityManager& entityManager, unsigned int bossID, BossComponent& boss)
{
	if (const TransformComponent* transform = entityManager.GetComponent<TransformComponent>(bossID))
		boss.restingScale = transform->scale;

	boss.restingGlows.clear();
	for (unsigned int entityID : GetBossAndChildren(entityManager, bossID))
	{
		if (const ModelComponent* model = entityManager.GetComponent<ModelComponent>(entityID))
			boss.restingGlows.push_back({ entityID, model->emissiveStrength });
	}

	if (const EnemyComponent* enemy = entityManager.GetComponent<EnemyComponent>(bossID))
		boss.restingHearingDistance = enemy->hearingDistance;

	if (const MeleeEnemyComponent* melee = entityManager.GetComponent<MeleeEnemyComponent>(bossID))
		boss.restingAttackCooldown = melee->attackCooldownSeconds;

	if (const HitboxComponent* weapon = FindWeapon(entityManager, bossID))
		boss.restingWeaponDamage = weapon->damage;

	if (const CharacterActionsComponent* actions = entityManager.GetComponent<CharacterActionsComponent>(bossID))
	{
		boss.restingLight1Animation = actions->lightAttack1Animation;
		boss.restingLight2Animation = actions->lightAttack2Animation;
		boss.restingLight3Animation = actions->lightAttack3Animation;
		boss.restingHeavyAnimation  = actions->heavyAttackAnimation;
		boss.restingLightHitFrom    = actions->lightHitFrom;
		boss.restingLightHitTo      = actions->lightHitTo;
		boss.restingHeavyHitFrom    = actions->heavyHitFrom;
		boss.restingHeavyHitTo      = actions->heavyHitTo;
		boss.restingShockwaveRadius = actions->shockwaveRadius;
	}

	if (const MeleeEnemyComponent* melee = entityManager.GetComponent<MeleeEnemyComponent>(bossID))
		boss.restingMaxLightCombo = melee->maxLightCombo;
}

void SetAwake(EntityManager& entityManager, unsigned int bossID, BossComponent& boss, bool isAwake)
{
	boss.isAwake = isAwake;

	if (EnemyComponent* enemy = entityManager.GetComponent<EnemyComponent>(bossID))
		enemy->hearingDistance = isAwake ? AwakeHearingDistance : boss.restingHearingDistance;
}

// The second phase's set of attacks (each one only when it is set), or back to the first phase's
void SwapAttacks(EntityManager& entityManager, unsigned int bossID, const BossComponent& boss, bool isInPhaseTwo)
{
	auto pick = [isInPhaseTwo](const std::string& phaseTwo, const std::string& resting)
	{
		return (isInPhaseTwo && !phaseTwo.empty()) ? phaseTwo : resting;
	};

	if (CharacterActionsComponent* actions = entityManager.GetComponent<CharacterActionsComponent>(bossID))
	{
		actions->lightAttack1Animation = pick(boss.phaseTwoLight1Animation, boss.restingLight1Animation);
		actions->lightAttack2Animation = pick(boss.phaseTwoLight2Animation, boss.restingLight2Animation);
		actions->lightAttack3Animation = pick(boss.phaseTwoLight3Animation, boss.restingLight3Animation);
		actions->heavyAttackAnimation  = pick(boss.phaseTwoHeavyAnimation,  boss.restingHeavyAnimation);

		const bool hasNewLights = isInPhaseTwo && !boss.phaseTwoLight1Animation.empty();
		const bool hasNewHeavy  = isInPhaseTwo && !boss.phaseTwoHeavyAnimation.empty();
		actions->lightHitFrom = hasNewLights ? boss.phaseTwoLightHitFrom : boss.restingLightHitFrom;
		actions->lightHitTo   = hasNewLights ? boss.phaseTwoLightHitTo   : boss.restingLightHitTo;
		actions->heavyHitFrom = hasNewHeavy  ? boss.phaseTwoHeavyHitFrom : boss.restingHeavyHitFrom;
		actions->heavyHitTo   = hasNewHeavy  ? boss.phaseTwoHeavyHitTo   : boss.restingHeavyHitTo;

		actions->shockwaveRadius = (isInPhaseTwo && boss.phaseTwoShockwaveRadius > 0.0f) ? boss.phaseTwoShockwaveRadius : boss.restingShockwaveRadius;
	}

	if (MeleeEnemyComponent* melee = entityManager.GetComponent<MeleeEnemyComponent>(bossID))
		melee->maxLightCombo = (isInPhaseTwo && boss.phaseTwoMaxLightCombo > 0) ? boss.phaseTwoMaxLightCombo : boss.restingMaxLightCombo;
}

// The volume of the battle music of the fog gate whose arena the boss stands in (the volume of any fog without one)
float GetBattleMusicVolume(EntityManager& entityManager, unsigned int bossID)
{
	float volume = FogGateComponent().battleMusicVolume;

	glm::vec3 position;
	if (!TryGetPosition(entityManager, bossID, position))
		return volume;

	for (const FogGateComponent& fog : entityManager.GetAllComponents<FogGateComponent>())
	{
		if (IsInside(fog, position))
			volume = fog.battleMusicVolume;
	}

	return volume;
}

// Heard when phase two starts: the roar from the boss, the music of the fight changes
void PlayPhaseTwoSounds(EntityManager& entityManager, unsigned int bossID, const BossComponent& boss)
{
	AudioSystem::PlayAtEntity(entityManager, bossID, boss.phaseTwoSound);

	if (!boss.phaseTwoMusic.empty())
		Audio::PlayMusic(boss.phaseTwoMusic, GetBattleMusicVolume(entityManager, bossID), BattleMusicFadeInSeconds);
}

void SetPhaseTwo(EntityManager& entityManager, unsigned int bossID, BossComponent& boss, bool isInPhaseTwo)
{
	boss.isInPhaseTwo = isInPhaseTwo;

	if (MeleeEnemyComponent* melee = entityManager.GetComponent<MeleeEnemyComponent>(bossID))
		melee->attackCooldownSeconds = isInPhaseTwo ? boss.restingAttackCooldown / boss.phaseTwoAttackSpeedup : boss.restingAttackCooldown;

	if (HitboxComponent* weapon = FindWeapon(entityManager, bossID))
		weapon->damage = isInPhaseTwo ? boss.restingWeaponDamage * boss.phaseTwoDamageMultiplier : boss.restingWeaponDamage;

	boss.phaseTwoSeconds = 0.0f;

	SwapAttacks(entityManager, bossID, boss, isInPhaseTwo);

	// The emitters start with a burst (the boss bursts into flames), back to how the scene had them when it rests again
	for (unsigned int entityID : GetBossAndChildren(entityManager, bossID))
	{
		if (ParticleEmitterComponent* emitter = entityManager.GetComponent<ParticleEmitterComponent>(entityID))
		{
			emitter->isEmitting = isInPhaseTwo || emitter->wasEmittingAtStart;
			if (isInPhaseTwo)
				emitter->burstCount += PhaseTwoBurst;
		}
	}

	if (isInPhaseTwo && !boss.phaseTwoAnimation.empty())
		CharacterActionsSystem::PlaySpecial(entityManager, bossID, boss.phaseTwoAnimation);

	if (isInPhaseTwo)
		PlayPhaseTwoSounds(entityManager, bossID, boss);

	if (!isInPhaseTwo)
	{
		if (TransformComponent* transform = entityManager.GetComponent<TransformComponent>(bossID))
		{
			transform->scale   = boss.restingScale;
			transform->isDirty = true;
		}

		for (const auto& [entityID, glow] : boss.restingGlows)
		{
			if (ModelComponent* model = entityManager.GetComponent<ModelComponent>(entityID))
				model->emissiveStrength = glow;
		}
	}
}

// Defeated: the emitters stop (the embers in the air live on) and the glow fades out
void GoOut(EntityManager& entityManager, unsigned int bossID, BossComponent& boss, float deltaTime)
{
	for (unsigned int entityID : GetBossAndChildren(entityManager, bossID))
	{
		if (ParticleEmitterComponent* emitter = entityManager.GetComponent<ParticleEmitterComponent>(entityID))
			emitter->isEmitting = false;
	}

	const float fadeStep = boss.phaseTwoGlow / GoingOutSeconds * deltaTime;
	for (const auto& [entityID, glow] : boss.restingGlows)
	{
		if (ModelComponent* model = entityManager.GetComponent<ModelComponent>(entityID))
			model->emissiveStrength = glm::max(model->emissiveStrength - fadeStep, 0.0f);
	}
}

// The look of phase two over time: growing during the first seconds, the glow pulsing like embers
void UpdatePhaseTwoLook(EntityManager& entityManager, unsigned int bossID, BossComponent& boss, float deltaTime)
{
	boss.phaseTwoSeconds += deltaTime;

	if (boss.phaseTwoGrowth != 1.0f)
	{
		if (TransformComponent* transform = entityManager.GetComponent<TransformComponent>(bossID))
		{
			const float grown = glm::clamp(boss.phaseTwoSeconds / GrowSeconds, 0.0f, 1.0f);
			const float smooth = grown * grown * (3.0f - 2.0f * grown);
			transform->scale = boss.restingScale * glm::mix(1.0f, boss.phaseTwoGrowth, smooth);
			transform->isDirty = true;
		}
	}

	if (boss.phaseTwoGlow > 0.0f)
	{
		// Glowing up from nothing at the start, then breathing
		const float rising = glm::clamp(boss.phaseTwoSeconds / GrowSeconds, 0.0f, 1.0f);
		const float pulse  = 1.0f + GlowPulse * glm::sin(boss.phaseTwoSeconds * GlowPulseSpeed);

		for (const auto& [entityID, glow] : boss.restingGlows)
		{
			if (ModelComponent* model = entityManager.GetComponent<ModelComponent>(entityID))
				model->emissiveStrength = boss.phaseTwoGlow * rising * pulse;
		}
	}
}

// Below the health limit the boss fights harder. Brought back with full health (the player died) it starts calm again
void UpdatePhase(EntityManager& entityManager, unsigned int bossID, BossComponent& boss)
{
	const HealthComponent* health = entityManager.GetComponent<HealthComponent>(bossID);
	if (!health)
		return;

	const bool isHurtEnough = health->currentHealth <= health->maxHealth * boss.phaseTwoHealthFraction;

	if (!boss.isInPhaseTwo && isHurtEnough && !health->isDead)
		SetPhaseTwo(entityManager, bossID, boss, true);
	else if (boss.isInPhaseTwo && health->currentHealth >= health->maxHealth)
		SetPhaseTwo(entityManager, bossID, boss, false);
}

// ========== The fog gates ==========

// A fog crossed with a key is a wall until somebody crosses it; otherwise it is a wall only while the fight goes on
void UpdateFogWall(EntityManager& entityManager, unsigned int fogID, const FogGateComponent& fog)
{
	const bool isCrossing = fog.crossingSeconds >= 0.0f;
	const bool isWall     = !fog.isCleared && !isCrossing && (fog.isClosed || fog.isCrossedWithKey);

	// A trigger collides with nobody
	if (BoxColliderComponent* wall = entityManager.GetComponent<BoxColliderComponent>(fogID))
		wall->isTrigger = !isWall;
}

void SetFogClosed(EntityManager& entityManager, unsigned int fogID, FogGateComponent& fog, bool isClosed)
{
	fog.isClosed = isClosed;
	UpdateFogWall(entityManager, fogID, fog);
}

void SetFogEmitters(EntityManager& entityManager, unsigned int fogID, bool isEmitting)
{
	std::vector<unsigned int> entities = { fogID };
	for (unsigned int childID : EntityHierarchy::GetChildren(entityManager, fogID))
		entities.push_back(childID);

	for (unsigned int entityID : entities)
	{
		if (ParticleEmitterComponent* emitter = entityManager.GetComponent<ParticleEmitterComponent>(entityID))
			emitter->isEmitting = isEmitting && emitter->wasEmittingAtStart;
	}
}

void ClearFog(EntityManager& entityManager, unsigned int fogID, FogGateComponent& fog)
{
	fog.isCleared = true;
	SetFogClosed(entityManager, fogID, fog, false);
	SetFogEmitters(entityManager, fogID, false);

	if (ModelComponent* model = entityManager.GetComponent<ModelComponent>(fogID))
		model->isVisible = false;

	if (!fog.victoryMessage.empty())
		HUDSystem::ShowNotification(entityManager, fog.victoryMessage);

	Audio::StopMusic(VictoryMusicFadeOutSeconds);
	Audio::Play(fog.victorySound);
}

// The music plays while the fog keeps the player in the arena, and goes quiet when it lets the player out (a death)
void UpdateBattleMusic(const FogGateComponent& fog)
{
	if (fog.isClosed)
		Audio::PlayMusic(fog.battleMusic, fog.battleMusicVolume, BattleMusicFadeInSeconds);
	else
		Audio::StopMusic(BattleMusicFadeOutSeconds);
}

// Every boss standing in the arena of the fog wakes up (or rests again) with it
void WakeBossesOfArena(EntityManager& entityManager, const FogGateComponent& fog, bool isAwake)
{
	for (BossComponent& boss : entityManager.GetAllComponents<BossComponent>())
	{
		const unsigned int bossID = entityManager.GetEntityIDForComponent(boss);

		glm::vec3 position;
		if (boss.isAwake != isAwake && TryGetPosition(entityManager, bossID, position) && IsInside(fog, position))
			SetAwake(entityManager, bossID, boss, isAwake);
	}
}

bool HasLivingBoss(EntityManager& entityManager, const FogGateComponent& fog)
{
	for (BossComponent& boss : entityManager.GetAllComponents<BossComponent>())
	{
		const unsigned int bossID = entityManager.GetEntityIDForComponent(boss);

		glm::vec3 position;
		if (IsAlive(entityManager, bossID) && TryGetPosition(entityManager, bossID, position) && IsInside(fog, position))
			return true;
	}

	return false;
}

// The player walks through the fog (its walking animation, carried from one side to the other), then plays on
void UpdateCrossing(EntityManager& entityManager, unsigned int fogID, FogGateComponent& fog, unsigned int playerID, float deltaTime)
{
	TransformComponent*   transform = entityManager.GetComponent<TransformComponent>(playerID);
	PhysicsBodyComponent* body      = entityManager.GetComponent<PhysicsBodyComponent>(playerID);
	if (!transform)
	{
		fog.crossingSeconds = -1.0f;
		return;
	}

	fog.crossingSeconds += deltaTime;
	const float done = glm::clamp(fog.crossingSeconds / CrossSeconds, 0.0f, 1.0f);

	transform->position = glm::mix(fog.crossingFrom, fog.crossingTo, done);
	transform->isDirty  = true;
	if (body)
		body->velocity = glm::vec3(0.0f, body->velocity.y, 0.0f);

	if (done >= 1.0f)
	{
		fog.crossingSeconds = -1.0f;
		UpdateFogWall(entityManager, fogID, fog);
	}
}

// Close to a fog crossed with a key (from outside the arena): the prompt, and E starts the walk through it
void OfferCrossing(EntityManager& entityManager, unsigned int fogID, FogGateComponent& fog, unsigned int playerID, const glm::vec3& playerPosition)
{
	glm::vec3 fogPosition;
	if (!TryGetPosition(entityManager, fogID, fogPosition))
		return;

	glm::vec3 toFog = fogPosition - playerPosition;
	toFog.y = 0.0f;
	if (glm::length(toFog) > CrossReachDistance)
		return;

	HUDSystem::ShowPrompt(entityManager, CrossPrompt);
	if (!InputManager::IsKeyDown(CrossKey))
		return;

	// Straight through the doorway towards the arena, coming out a little behind the fog
	glm::vec3 inward = fog.arenaCenter - fogPosition;
	inward.y = 0.0f;
	inward = glm::length(inward) > 0.0f ? glm::normalize(inward) : glm::vec3(0.0f, 0.0f, -1.0f);
	if (std::abs(inward.x) > std::abs(inward.z))
		inward = glm::vec3(glm::sign(inward.x), 0.0f, 0.0f);
	else
		inward = glm::vec3(0.0f, 0.0f, glm::sign(inward.z));

	fog.crossingFrom    = playerPosition;
	fog.crossingTo      = glm::vec3(fogPosition.x, playerPosition.y, fogPosition.z) + inward * CrossBehindFog;
	fog.crossingSeconds = 0.0f;
	UpdateFogWall(entityManager, fogID, fog);

	TransformComponent*                 transform  = entityManager.GetComponent<TransformComponent>(playerID);
	const CharacterControllerComponent* controller = entityManager.GetComponent<CharacterControllerComponent>(playerID);
	if (transform && controller)
	{
		transform->rotation = glm::vec3(0.0f, GameplayMath::WrapDegrees(GameplayMath::GetYawToFace(inward) + controller->facingOffsetDegrees), 0.0f);
		transform->SyncQuaternionFromEuler();
		transform->isDirty = true;
	}

	if (const CharacterAnimationComponent* animation = entityManager.GetComponent<CharacterAnimationComponent>(playerID))
		CharacterActionsSystem::PlaySpecial(entityManager, playerID, animation->walkAnimation);
}

void UpdateFog(EntityManager& entityManager, unsigned int fogID, FogGateComponent& fog, bool isPlayerAliveInside)
{
	if (fog.isCleared)
		return;

	if (!HasLivingBoss(entityManager, fog))
	{
		// Every boss of the arena fell during the fight (an arena that never had one keeps its fog)
		if (fog.isClosed)
			ClearFog(entityManager, fogID, fog);
		return;
	}

	if (fog.isClosed == isPlayerAliveInside)
		return;

	SetFogClosed(entityManager, fogID, fog, isPlayerAliveInside);
	WakeBossesOfArena(entityManager, fog, isPlayerAliveInside);
	UpdateBattleMusic(fog);

	// The boss shows itself the first time (a cutscene plays only once)
	if (isPlayerAliveInside && !fog.cutscene.empty())
		CutsceneSystem::PlayByName(entityManager, fog.cutscene);
}

}

void BossSystem::PrepareForPlay(EntityManager& entityManager)
{
	for (BossComponent& boss : entityManager.GetAllComponents<BossComponent>())
	{
		const unsigned int bossID = entityManager.GetEntityIDForComponent(boss);

		boss.isAwake      = false;
		boss.isInPhaseTwo = false;
		boss.isDefeated   = false;
		RememberRestingValues(entityManager, bossID, boss);
		Audio::Preload(boss.phaseTwoSound);
	}

	for (FogGateComponent& fog : entityManager.GetAllComponents<FogGateComponent>())
	{
		const unsigned int fogID = entityManager.GetEntityIDForComponent(fog);

		fog.isCleared       = false;
		fog.crossingSeconds = -1.0f;
		SetFogClosed(entityManager, fogID, fog, false);
		Audio::Preload(fog.victorySound);
		SetFogEmitters(entityManager, fogID, true);

		if (ModelComponent* model = entityManager.GetComponent<ModelComponent>(fogID))
			model->isVisible = true;
	}
}

void BossSystem::Update(EntityManager& entityManager, float deltaTime)
{
	for (BossComponent& boss : entityManager.GetAllComponents<BossComponent>())
	{
		const unsigned int bossID = entityManager.GetEntityIDForComponent(boss);

		if (!IsAlive(entityManager, bossID))
			boss.isDefeated = true;

		UpdatePhase(entityManager, bossID, boss);

		if (boss.isDefeated)
			GoOut(entityManager, bossID, boss, deltaTime);
		else if (boss.isInPhaseTwo)
			UpdatePhaseTwoLook(entityManager, bossID, boss, deltaTime);
	}

	unsigned int playerID = 0;
	glm::vec3    playerPosition(0.0f);
	const bool   hasPlayer = CharacterControllerSystem::TryFindPlayer(entityManager, playerID) && TryGetPosition(entityManager, playerID, playerPosition);
	const bool   isPlayerAlive = hasPlayer && IsAlive(entityManager, playerID);

	for (FogGateComponent& fog : entityManager.GetAllComponents<FogGateComponent>())
	{
		const unsigned int fogID    = entityManager.GetEntityIDForComponent(fog);
		const bool         isInside = isPlayerAlive && IsInside(fog, playerPosition);

		if (fog.crossingSeconds >= 0.0f)
			UpdateCrossing(entityManager, fogID, fog, playerID, deltaTime);
		else if (fog.isCrossedWithKey && !fog.isCleared && !fog.isClosed && isPlayerAlive && !isInside)
			OfferCrossing(entityManager, fogID, fog, playerID, playerPosition);

		UpdateFog(entityManager, fogID, fog, isInside);
	}
}

bool BossSystem::CanBeBroughtBack(EntityManager& entityManager, unsigned int enemyID)
{
	const BossComponent* boss = entityManager.GetComponent<BossComponent>(enemyID);
	return !boss || !boss->isDefeated;
}

}
