#pragma once

#include <map>
#include <memory>
#include <string>

#include "EnemyState.h"

namespace Sunta
{

// The states of one enemy and the one it is in now
// The first update enters the starting state, after that a state's Update decides when to switch
class EnemyStateMachine
{
public:
	explicit EnemyStateMachine(std::string startingStateName);

	// Adds the state, or replaces the one with the same name (a script overriding a ready state)
	void SetState(const std::string& name, std::unique_ptr<EnemyState> state);
	bool HasState(const std::string& name) const;

	// Switches in the next update (e.g. a script, or the death). An unknown name is ignored with a warning
	void RequestState(const std::string& name);

	void Update(EnemyContext& context);

	const std::string& GetCurrentStateName() const { return currentStateName; }
	float GetSecondsInState() const { return secondsInState; }

private:
	std::map<std::string, std::unique_ptr<EnemyState>> states;

	std::string currentStateName;
	std::string requestedStateName;
	float secondsInState = 0.0f;

	EnemyState* FindState(const std::string& name);
	void SwitchTo(const std::string& name, EnemyContext& context);
};

}
