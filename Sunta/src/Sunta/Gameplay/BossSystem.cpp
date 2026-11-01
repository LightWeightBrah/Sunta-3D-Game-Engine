#include "Core/SuntaPreCompiled.h"
#include "BossSystem.h"

#include <glm/glm.hpp>

#include "CharacterControllerSystem.h"
#include "HUDSystem.h"
#include "ECS/Component.h"
#include "ECS/EntityManager.h"
#include "Scene/EntityHierarchy.h"

namespace Sunta
{

namespace
{

// An awake boss hears the player anywhere in its arena, so it never stands waiting while the player walks around it
constexpr float AwakeHearingDistance = 100.0f;

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

void RememberRestingValues(EntityManager& entityManager, unsigned int bossID, BossComponent& boss)
{
	if (const EnemyComponent* enemy = entityManager.GetComponent<EnemyComponent>(bossID))
		boss.restingHearingDistance = enemy->hearingDistance;

	if (const MeleeEnemyComponent* melee = entityManager.GetComponent<MeleeEnemyComponent>(bossID))
		boss.restingAttackCooldown = melee->attackCooldownSeconds;

	if (const HitboxComponent* weapon = FindWeapon(entityManager, bossID))
		boss.restingWeaponDamage = weapon->damage;
}

void SetAwake(EntityManager& entityManager, unsigned int bossID, BossComponent& boss, bool isAwake)
{
	boss.isAwake = isAwake;

	if (EnemyComponent* enemy = entityManager.GetComponent<EnemyComponent>(bossID))
		enemy->hearingDistance = isAwake ? AwakeHearingDistance : boss.restingHearingDistance;
}

void SetPhaseTwo(EntityManager& entityManager, unsigned int bossID, BossComponent& boss, bool isInPhaseTwo)
{
	boss.isInPhaseTwo = isInPhaseTwo;

	if (MeleeEnemyComponent* melee = entityManager.GetComponent<MeleeEnemyComponent>(bossID))
		melee->attackCooldownSeconds = isInPhaseTwo ? boss.restingAttackCooldown / boss.phaseTwoAttackSpeedup : boss.restingAttackCooldown;

	if (HitboxComponent* weapon = FindWeapon(entityManager, bossID))
		weapon->damage = isInPhaseTwo ? boss.restingWeaponDamage * boss.phaseTwoDamageMultiplier : boss.restingWeaponDamage;
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

void SetFogClosed(EntityManager& entityManager, unsigned int fogID, FogGateComponent& fog, bool isClosed)
{
	fog.isClosed = isClosed;

	// A trigger collides with nobody: the player walks through the fog until it closes behind it
	if (BoxColliderComponent* wall = entityManager.GetComponent<BoxColliderComponent>(fogID))
		wall->isTrigger = !isClosed;
}

void ClearFog(EntityManager& entityManager, unsigned int fogID, FogGateComponent& fog)
{
	SetFogClosed(entityManager, fogID, fog, false);
	fog.isCleared = true;

	if (ModelComponent* model = entityManager.GetComponent<ModelComponent>(fogID))
		model->isVisible = false;

	if (!fog.victoryMessage.empty())
		HUDSystem::ShowNotification(entityManager, fog.victoryMessage);
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
	}

	for (FogGateComponent& fog : entityManager.GetAllComponents<FogGateComponent>())
	{
		const unsigned int fogID = entityManager.GetEntityIDForComponent(fog);

		fog.isCleared = false;
		SetFogClosed(entityManager, fogID, fog, false);

		if (ModelComponent* model = entityManager.GetComponent<ModelComponent>(fogID))
			model->isVisible = true;
	}
}

void BossSystem::Update(EntityManager& entityManager)
{
	for (BossComponent& boss : entityManager.GetAllComponents<BossComponent>())
	{
		const unsigned int bossID = entityManager.GetEntityIDForComponent(boss);

		if (!IsAlive(entityManager, bossID))
			boss.isDefeated = true;

		UpdatePhase(entityManager, bossID, boss);
	}

	unsigned int playerID = 0;
	glm::vec3    playerPosition(0.0f);
	const bool   hasPlayer = CharacterControllerSystem::TryFindPlayer(entityManager, playerID) && TryGetPosition(entityManager, playerID, playerPosition);
	const bool   isPlayerAlive = hasPlayer && IsAlive(entityManager, playerID);

	for (FogGateComponent& fog : entityManager.GetAllComponents<FogGateComponent>())
		UpdateFog(entityManager, entityManager.GetEntityIDForComponent(fog), fog, isPlayerAlive && IsInside(fog, playerPosition));
}

bool BossSystem::CanBeBroughtBack(EntityManager& entityManager, unsigned int enemyID)
{
	const BossComponent* boss = entityManager.GetComponent<BossComponent>(enemyID);
	return !boss || !boss->isDefeated;
}

}
