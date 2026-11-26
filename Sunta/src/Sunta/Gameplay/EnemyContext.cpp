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
#include "Core/Log.h"
#include <unordered_map>

namespace
{
// How far an ally looks for enemies to fight, and how much nearer than the player an ally must be to be fought first
constexpr float AllyTargetRange      = 22.0f;
constexpr float AllyPreferenceMeters = 1.5f;
constexpr unsigned int NoFighter     = 0xFFFFFFFFu;
}

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

// The nearest living, awake fighter of the given side within 'range' of 'from' (NoFighter when there is none;
// 0 is a real entity)
static unsigned int FindNearestFighter(EntityManager& entityManager, const glm::vec3& from, bool allies, float range, unsigned int self)
{
	unsigned int nearest = NoFighter;
	float nearestDistance = range;
	for (EnemyComponent& other : entityManager.GetAllComponents<EnemyComponent>())
	{
		const unsigned int otherID = entityManager.GetEntityIDForComponent(other);
		if (otherID == self || other.isAlly != allies || other.isDormant || !IsAlive(entityManager, otherID))
			continue;
		const TransformComponent* transform = entityManager.GetComponent<TransformComponent>(otherID);
		if (!transform)
			continue;
		const float distance = glm::length(KeepOnlyHorizontalPart(transform->position - from));
		if (distance < nearestDistance)
		{
			nearestDistance = distance;
			nearest = otherID;
		}
	}
	return nearest;
}

void EnemyContext::FindTarget()
{
	unsigned int playerID = 0;
	const bool hasPlayer = CharacterControllerSystem::TryFindPlayer(entityManager, playerID) && playerID != entityID
	                       && IsAlive(entityManager, playerID);

	// An ally fights the nearest enemy (none about: no target, it keeps near the player, see EnemySystem)
	unsigned int chosen = NoFighter;
	if (enemy->isAlly)
		chosen = FindNearestFighter(entityManager, transform->position, false, AllyTargetRange, entityID);
	else
	{
		// An enemy goes for the player, or for an ally of his that stands clearly nearer
		chosen = hasPlayer ? playerID : NoFighter;
		const unsigned int ally = FindNearestFighter(entityManager, transform->position, true, AllyTargetRange, entityID);
		if (ally != NoFighter)
		{
			const TransformComponent* allyTransform   = entityManager.GetComponent<TransformComponent>(ally);
			const TransformComponent* playerTransform = hasPlayer ? entityManager.GetComponent<TransformComponent>(playerID) : nullptr;
			const float toAlly   = glm::length(KeepOnlyHorizontalPart(allyTransform->position - transform->position));
			const float toPlayer = playerTransform ? glm::length(KeepOnlyHorizontalPart(playerTransform->position - transform->position))
			                                       : std::numeric_limits<float>::max();
			if (toAlly + AllyPreferenceMeters < toPlayer)
				chosen = ally;
		}
	}
	if (chosen == NoFighter)
		return;

	const TransformComponent* targetTransform = entityManager.GetComponent<TransformComponent>(chosen);
	if (!targetTransform)
		return;

	hasTarget      = true;
	targetID       = chosen;
	targetPosition = targetTransform->position;
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

	// A summoned ally knows where the enemies about it are (it was called to fight them)
	if (enemy->isAlly)
		return distance <= enemy->sightDistance;

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

bool EnemyContext::IsTargetAttacking() const
{
	const CharacterActionsComponent* targetActions = hasTarget ? entityManager.GetComponent<CharacterActionsComponent>(targetID) : nullptr;
	return targetActions && targetActions->IsAttacking();
}

unsigned int EnemyContext::GetTargetAttackNumber() const
{
	const CharacterActionsComponent* targetActions = hasTarget ? entityManager.GetComponent<CharacterActionsComponent>(targetID) : nullptr;
	return targetActions ? targetActions->attackNumber : 0;
}

bool EnemyContext::HasRoll() const
{
	return actions && !actions->rollAnimation.empty();
}

bool EnemyContext::HasBackstep() const
{
	return actions && !actions->backstepAnimation.empty();
}

bool EnemyContext::IsDodgeReady() const
{
	return enemy->dodgeCooldownLeft <= 0.0f;
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

bool EnemyContext::IsMoving() const
{
	return glm::length(controller->moveDirection) > 0.0f;
}

bool EnemyContext::IsRunning() const
{
	return IsMoving() && controller->wantsToRun;
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

void EnemyContext::Dodge(const glm::vec3& direction)
{
	// The roll goes where the "stick" is held; with nothing held the Character Actions make it a backstep
	controller->moveDirection = KeepOnlyHorizontalPart(direction);
	controller->wantsToRun    = false;

	if (actions)
		actions->requestedAction = CharacterAction::Roll;
}

void EnemyContext::StartDodgeCooldown(float seconds)
{
	enemy->dodgeCooldownLeft = seconds;
}

void EnemyContext::StartAttackCooldown(float seconds)
{
	enemy->attackCooldownLeft = seconds;
}

}
