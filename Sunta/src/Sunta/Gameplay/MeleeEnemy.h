#pragma once

namespace Sunta
{

class EntityManager;
class EnemyStateMachine;

// The names of the states a Melee Enemy brings (on top of Idle and Dead of every enemy, see BaseEnemyStates.h)
namespace MeleeEnemyStateNames
{
	constexpr const char* Chase      = "Chase";      // walks (or runs) to the player, facing it
	constexpr const char* Attack     = "Attack";     // a light combo or a heavy attack
	constexpr const char* Recover    = "Recover";    // circles the player for a moment after attacking
	constexpr const char* ReturnHome = "ReturnHome"; // lost the player: walks back to where it started
}

// The sword fighter kind of enemy (see MeleeEnemyComponent)
class MeleeEnemy
{
public:
	// Replaces Idle (now it waits until it sees the player) and adds Chase, Attack, Recover and ReturnHome
	static void AddStates(EnemyStateMachine& stateMachine);

	// Adds Melee Enemy together with the Enemy it is built on (and what that one needs)
	static void AddWithRequiredComponents(EntityManager& entityManager, unsigned int entityID);
};

}
