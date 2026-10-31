#pragma once

#include <string>
#include <glm/glm.hpp>

#include "ECS/EntityManager.h"

namespace Sunta
{

struct EnemyComponent;
struct CharacterControllerComponent;
struct CharacterActionsComponent;
struct TransformComponent;
enum class CharacterAction;

// What a state of an enemy knows and what it can do, built for one enemy for one frame
// The commands only set what the Character Controller and the Character Actions do next, so an enemy moves
// exactly like the player (same speeds, same animations, same rolls and attacks)
class EnemyContext
{
public:
	EnemyContext(EntityManager& entityManager, unsigned int entityID, float deltaTime);

	// False when the enemy lacks a part it needs (Enemy, Character Controller, Transform): then nothing else may be called
	bool IsValid() const;

	EntityManager&  GetEntityManager() const { return entityManager; }
	unsigned int    GetEntityID()      const { return entityID; }
	float           GetDeltaTime()     const { return deltaTime; }
	EnemyComponent& GetEnemy()         const { return *enemy; }

	// Another component of the enemy (e.g. the settings of its kind), nullptr when it doesn't have one
	template<typename TComponent>
	TComponent* TryGet() const { return entityManager.GetComponent<TComponent>(entityID); }

	// ========== What the enemy knows ==========

	// The target is the player (the first player-controlled Character Controller), while the player is alive
	bool         HasTarget()             const { return hasTarget; }
	unsigned int GetTargetID()           const { return targetID; }
	glm::vec3    GetTargetPosition()     const { return targetPosition; }
	float        GetDistanceToTarget()   const;
	float        GetDistanceToHome()     const;
	glm::vec3    GetPosition()           const;

	// In front within the sight distance and not behind a wall, or closer than the hearing distance
	bool CanSeeTarget() const;

	// True while a roll, an attack or a hit reaction plays (the enemy can't walk then)
	bool IsBusy() const;
	bool IsDead() const;

	float GetSecondsInState() const;
	bool  IsAttackReady()     const;

	// Grows with every attack the enemy starts (see CharacterActionsComponent::attackNumber)
	unsigned int GetAttackNumber() const;

	// A number from 0 to 1, different every time (for choosing between attacks)
	static float GetRandomFraction();

	// ========== What the enemy can do ==========

	void MoveTowards(const glm::vec3& point, bool run);

	// Walks sideways around the target (circling), 'side' +1 to its right, -1 to its left
	void CircleTarget(float side);

	void Stop();

	// While on, the enemy keeps facing the target when it walks (uses its Lock On)
	void LookAtTarget(bool shouldLook);

	// Asks the Character Actions for an attack (or a roll). Pressed during another attack it chains as a combo
	void RequestAction(CharacterAction action);

	// No attack is allowed for that long (IsAttackReady is false until then)
	void StartAttackCooldown(float seconds);

private:
	EntityManager& entityManager;
	unsigned int   entityID;
	float          deltaTime;

	EnemyComponent*               enemy      = nullptr;
	CharacterControllerComponent* controller = nullptr;
	CharacterActionsComponent*    actions    = nullptr;
	TransformComponent*           transform  = nullptr;

	bool         hasTarget      = false;
	unsigned int targetID       = 0;
	glm::vec3    targetPosition = glm::vec3(0.0f);

	void FindTarget();
	glm::vec3 GetFacingDirection() const;
	bool IsTargetInFront() const;
	bool IsTargetBehindWall() const;
};

}
