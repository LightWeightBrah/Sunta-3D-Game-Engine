#pragma once

namespace Sunta
{

class EntityManager;
class EnemyStateMachine;

// Runs the state machine of every enemy (an entity with an EnemyComponent)
//
// The states of an enemy come from: the basic ones (BaseEnemyStates.h), then the ones of its kind (e.g. Melee Enemy),
// then whatever a script adds. A new kind of enemy = a component, a class that adds its states, and one line in
// EnemyKinds (EnemySystem.cpp)
class EnemySystem
{
public:
	// Call when the game starts: every enemy remembers where it stands (its home) and starts in Idle
	static void PrepareForPlay(EntityManager& entityManager);

	// Call once per frame AFTER the scripts (they may add states) and BEFORE the Character Controller (it moves the enemy)
	static void Update(EntityManager& entityManager, float deltaTime);

	// The state machine of the enemy, made with the states of its kind the first time it is asked for. nullptr for a non-enemy
	static EnemyStateMachine* GetOrCreateStateMachine(EntityManager& entityManager, unsigned int entityID);

	// Adds Enemy together with what an enemy needs: a Character Controller not driven by the keyboard and Lock On (to face the player)
	static void AddWithRequiredComponents(EntityManager& entityManager, unsigned int entityID);

	// Call when the game starts: explains in the log why an enemy would stand still
	static void ReportProblemsBeforePlay(EntityManager& entityManager);
};

}
