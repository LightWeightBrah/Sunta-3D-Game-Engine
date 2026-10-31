#include "Core/SuntaPreCompiled.h"
#include "LuaEnemyState.h"

#include "EnemyContext.h"
#include "Core/Log.h"
#include "ECS/Entity.h"

namespace Sunta
{

namespace
{

constexpr const char* EnterFunctionName  = "enter";
constexpr const char* UpdateFunctionName = "update";
constexpr const char* ExitFunctionName   = "exit";

sol::protected_function GetFunction(const sol::table& functions, const char* name)
{
	const sol::object value = functions[name];
	return value.is<sol::function>() ? value.as<sol::protected_function>() : sol::protected_function();
}

Entity GetEnemy(const EnemyContext& context)
{
	return Entity(context.GetEntityID(), &context.GetEntityManager());
}

}

LuaEnemyState::LuaEnemyState(const sol::table& functions)
	: enterFunction (GetFunction(functions, EnterFunctionName))
	, updateFunction(GetFunction(functions, UpdateFunctionName))
	, exitFunction  (GetFunction(functions, ExitFunctionName))
{
}

void LuaEnemyState::Enter(EnemyContext& context)
{
	Call(enterFunction, EnterFunctionName, GetEnemy(context));
}

std::string LuaEnemyState::Update(EnemyContext& context)
{
	const sol::object nextState = Call(updateFunction, UpdateFunctionName, GetEnemy(context), context.GetDeltaTime());
	return nextState.is<std::string>() ? nextState.as<std::string>() : std::string();
}

void LuaEnemyState::Exit(EnemyContext& context)
{
	Call(exitFunction, ExitFunctionName, GetEnemy(context));
}

template<typename... Args>
sol::object LuaEnemyState::Call(sol::protected_function& function, const char* name, Args&&... args)
{
	if (!function.valid())
		return sol::nil;

	sol::protected_function_result result = function(std::forward<Args>(args)...);
	if (result.valid())
		return result.get<sol::object>();

	sol::error error = result;
	SUNTA_ENGINE_LOG_ERROR("Lua error in the '{0}' function of an enemy state: {1}. That function is turned off", name, error.what());

	function = sol::protected_function();
	return sol::nil;
}

}
