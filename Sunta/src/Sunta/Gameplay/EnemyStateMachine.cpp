#include "Core/SuntaPreCompiled.h"
#include "EnemyStateMachine.h"

#include "EnemyContext.h"
#include "Core/Log.h"

namespace Sunta
{

EnemyStateMachine::EnemyStateMachine(std::string startingStateName)
	: requestedStateName(std::move(startingStateName))
{
}

void EnemyStateMachine::SetState(const std::string& name, std::unique_ptr<EnemyState> state)
{
	states[name] = std::move(state);
}

bool EnemyStateMachine::HasState(const std::string& name) const
{
	return states.count(name) > 0;
}

void EnemyStateMachine::RequestState(const std::string& name)
{
	if (!HasState(name))
	{
		SUNTA_ENGINE_LOG_WARNING("Enemy AI: there is no state called '{0}'", name);
		return;
	}

	requestedStateName = name;
}

void EnemyStateMachine::Update(EnemyContext& context)
{
	if (!requestedStateName.empty())
	{
		const std::string nextStateName = requestedStateName;
		requestedStateName.clear();
		SwitchTo(nextStateName, context);
	}

	EnemyState* state = FindState(currentStateName);
	if (!state)
		return;

	secondsInState += context.GetDeltaTime();

	const std::string nextStateName = state->Update(context);

	if (!nextStateName.empty() && nextStateName != currentStateName)
		RequestState(nextStateName);
}

EnemyState* EnemyStateMachine::FindState(const std::string& name)
{
	const auto found = states.find(name);
	return found != states.end() ? found->second.get() : nullptr;
}

void EnemyStateMachine::SwitchTo(const std::string& name, EnemyContext& context)
{
	EnemyState* nextState = FindState(name);
	if (!nextState)
		return;

	if (EnemyState* currentState = FindState(currentStateName))
		currentState->Exit(context);

	currentStateName = name;
	secondsInState   = 0.0f;

	nextState->Enter(context);
}

}
