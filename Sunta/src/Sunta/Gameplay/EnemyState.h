#pragma once

#include <string>

namespace Sunta
{

class EnemyContext;

// One state of an enemy (Idle, Chase, Attack, ...). A new kind of behaviour is a new class derived from this one
// (or a Lua table, see LuaEnemyState), added to the enemy's EnemyStateMachine under a name
//
// Every function gets the context: what the enemy knows (the player, distances, sight) and what it can do (move, attack)
class EnemyState
{
public:
	virtual ~EnemyState() = default;

	// Once, when the enemy switches to this state
	virtual void Enter(EnemyContext& context) {}

	// Every frame while the state is current. Returns the name of the next state, or an empty string to stay
	virtual std::string Update(EnemyContext& context) = 0;

	// Once, when the enemy leaves this state
	virtual void Exit(EnemyContext& context) {}
};

}
