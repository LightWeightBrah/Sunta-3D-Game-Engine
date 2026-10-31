#include "Core/SuntaPreCompiled.h"
#include "BaseEnemyStates.h"

#include "EnemyContext.h"
#include "EnemyStateMachine.h"

namespace Sunta
{

namespace
{

// Stands where it is and lets go of the player
class StandStillState : public EnemyState
{
public:
	void Enter(EnemyContext& context) override
	{
		context.Stop();
		context.LookAtTarget(false);
	}

	std::string Update(EnemyContext& context) override
	{
		context.Stop();
		return "";
	}
};

}

void AddBaseEnemyStates(EnemyStateMachine& stateMachine)
{
	stateMachine.SetState(EnemyStateNames::Idle, std::make_unique<StandStillState>());
	stateMachine.SetState(EnemyStateNames::Dead, std::make_unique<StandStillState>());
}

}
