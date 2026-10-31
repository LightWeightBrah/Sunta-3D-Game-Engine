#pragma once

namespace Sunta
{

class EnemyStateMachine;

// The states every enemy has, whatever its kind. A kind (or a script) replaces them by adding a state under the same name
namespace EnemyStateNames
{
	constexpr const char* Idle = "Idle"; // the first state: stands still (a kind replaces it with "wait until the player is seen")
	constexpr const char* Dead = "Dead"; // entered when the Health runs out, the enemy never acts again
}

void AddBaseEnemyStates(EnemyStateMachine& stateMachine);

}
