#include "Core/SuntaPreCompiled.h"
#include "EnemyContext.h"

#include <random>

#include "CharacterControllerSystem.h"
#include "EnemyStateMachine.h"
#include "GameplayMath.h"
#include "LineOfSight.h"
#include "LockOnSystem.h"
#include "ECS/Component.h"
#include "ECS/EntityManager.h"

namespace Sunta
{

namespace
{

using namespace GameplayMath;

// The target is looked at at this height above its feet (its chest)
constexpr float TargetEyeHeight = 1.4f;

// How much of the full speed circling uses (a slow, careful side step)
constexpr float CircleSpeedFraction = 0.6f;

bool IsAlive(EntityManager& entityManager, unsigned int entityID)
{
	const HealthComponent* health = entityManager.GetComponent<HealthComponent>(entityID);
	return !health || !health->isDead;
}

}

EnemyContext::EnemyContext(EntityManager& entityManager, unsigned int entityID, float deltaTime)
	: entityManager(entityManager)
	, entityID(entityID)
	, deltaTime(deltaTime)
	, enemy(entityManager.GetComponent<EnemyComponent>(entityID))
	, controller(entityManager.GetComponent<CharacterControllerComponent>(entityID))
	, actions(entityManager.GetComponent<CharacterActionsComponent>(entityID))
	, transform(entityManager.GetComponent<TransformComponent>(entityID))
{
	if (IsValid())
		FindTarget();
}

bool EnemyContext::IsValid() const
{
	return enemy && controller && transform;
}

void EnemyContext::FindTarget()
{
	unsigned int playerID = 0;
	if (!CharacterControllerSystem::TryFindPlayer(entityManager, playerID) || playerID == entityID || !IsAlive(entityManager, playerID))
		return;

	const TransformComponent* playerTransform = entityManager.GetComponent<TransformComponent>(playerID);
	if (!playerTransform)
		return;

	hasTarget      = true;
	targetID       = playerID;
	targetPosition = playerTransform->position;
}

// ========== What the enemy knows ==========

glm::vec3 EnemyContext::GetPosition() const
{
	return transform->position;
}

float EnemyContext::GetDistanceToTarget() const
{
	return hasTarget ? glm::length(KeepOnlyHorizontalPart(targetPosition - transform->position)) : std::numeric_limits<float>::max();
}

float EnemyContext::GetDistanceToHome() const
{
	return glm::length(KeepOnlyHorizontalPart(enemy->homePosition - transform->position));
}

glm::vec3 EnemyContext::GetFacingDirection() const
{
	return GetDirectionFromYawAndPitch(transform->rotation.y - controller->facingOffsetDegrees, 0.0f);
}

bool EnemyContext::IsTargetInFront() const
{
	const glm::vec3 toTarget = GetHorizontalDirectionOrDefault(targetPosition - transform->position);
	const float cosine = glm::dot(toTarget, GetFacingDirection());

	return cosine >= std::cos(glm::radians(enemy->sightAngleDegrees));
}

bool EnemyContext::IsTargetBehindWall() const
{
	const glm::vec3 eyes   = transform->position + WorldUp * enemy->eyeHeight;
	const glm::vec3 target = targetPosition + WorldUp * TargetEyeHeight;

	RaycastHit hit;
	return IsLineOfSightBlocked(entityManager, eyes, target, entityID, targetID, hit);
}

bool EnemyContext::CanSeeTarget() const
{
	if (!hasTarget)
		return false;

	const float distance = GetDistanceToTarget();

	if (distance <= enemy->hearingDistance)
		return true;

	return distance <= enemy->sightDistance && IsTargetInFront() && !IsTargetBehindWall();
}

bool EnemyContext::IsBusy() const
{
	return actions && actions->IsBusy();
}

bool EnemyContext::IsDead() const
{
	return !IsAlive(entityManager, entityID);
}

float EnemyContext::GetSecondsInState() const
{
	return enemy->stateMachine ? enemy->stateMachine->GetSecondsInState() : 0.0f;
}

bool EnemyContext::IsAttackReady() const
{
	return enemy->attackCooldownLeft <= 0.0f;
}

unsigned int EnemyContext::GetAttackNumber() const
{
	return actions ? actions->attackNumber : 0;
}

float EnemyContext::GetRandomFraction()
{
	static std::mt19937 generator(std::random_device{}());
	return std::uniform_real_distribution<float>(0.0f, 1.0f)(generator);
}

// ========== What the enemy can do ==========

void EnemyContext::MoveTowards(const glm::vec3& point, bool run)
{
	controller->moveDirection = GetHorizontalDirectionOrDefault(point - transform->position);
	controller->wantsToRun    = run;
}

void EnemyContext::CircleTarget(float side)
{
	const glm::vec3 toTarget = GetHorizontalDirectionOrDefault(targetPosition - transform->position);

	controller->moveDirection = glm::cross(toTarget, WorldUp) * side * CircleSpeedFraction;
	controller->wantsToRun    = false;
}

void EnemyContext::Stop()
{
	controller->moveDirection = glm::vec3(0.0f);
	controller->wantsToRun    = false;
}

void EnemyContext::LookAtTarget(bool shouldLook)
{
	LockOnComponent* lockOn = entityManager.GetComponent<LockOnComponent>(entityID);
	if (!lockOn)
		return;

	if (shouldLook && hasTarget)
		LockOnSystem::TrySetTarget(entityManager, *lockOn, targetID);
	else
		LockOnSystem::ClearTarget(*lockOn);
}

void EnemyContext::RequestAction(CharacterAction action)
{
	if (actions)
		actions->requestedAction = action;
}

void EnemyContext::StartAttackCooldown(float seconds)
{
	enemy->attackCooldownLeft = seconds;
}

}
