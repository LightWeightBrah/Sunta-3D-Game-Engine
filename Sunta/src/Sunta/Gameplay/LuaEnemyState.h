#pragma once

#include <sol/sol.hpp>

#include "EnemyState.h"

namespace Sunta
{

// A state of an enemy written in Lua: a table with the functions enter, update and exit (each one optional, update returns
// the name of the next state or nothing). Every function gets the enemy entity, update also the frame time:
//
//     this.enemy:addState("Taunt", {
//         enter  = function(entity) entity.enemy:stop() end,
//         update = function(entity, deltaTime)
//             if entity.enemy.secondsInState > 2 then return "Chase" end
//         end
//     })
//
// A Lua error is logged once and turns that function off (the enemy then just stays in the state)
class LuaEnemyState : public EnemyState
{
public:
	explicit LuaEnemyState(const sol::table& functions);

	void        Enter (EnemyContext& context) override;
	std::string Update(EnemyContext& context) override;
	void        Exit  (EnemyContext& context) override;

private:
	sol::protected_function enterFunction;
	sol::protected_function updateFunction;
	sol::protected_function exitFunction;

	template<typename... Args>
	sol::object Call(sol::protected_function& function, const char* name, Args&&... args);
};

}
