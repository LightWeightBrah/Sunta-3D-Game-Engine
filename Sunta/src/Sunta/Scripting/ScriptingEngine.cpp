#include "Core/SuntaPreCompiled.h"
#include "ScriptingEngine.h"
#include "Core/Assert.h"
#include "Core/Log.h"
#include "LuaBindings.h"

namespace Sunta
{

namespace
{

// print() of Lua writes to the standard output, which a game without a console doesn't have,
// so it writes to the game log instead (console and log file), like SUNTA_GAME_LOG_INFO
void RegisterPrintToGameLog(sol::state& lua)
{
	sol::function toString = lua["tostring"];

	lua.set_function("print", [toString](sol::variadic_args arguments)
	{
		std::string message;
		bool isFirstArgument = true;

		for (auto argument : arguments)
		{
			if (!isFirstArgument)
				message += "\t";

			const sol::object value = argument.get<sol::object>();
			message += toString(value).get<std::string>();
			isFirstArgument = false;
		}

		SUNTA_GAME_LOG_INFO("{0}", message);
	});
}

}

void ScriptingEngine::Init()
{
	luaState = new sol::state();
	luaState->open_libraries(sol::lib::base, sol::lib::math, sol::lib::table, sol::lib::os, sol::lib::string);
	RegisterPrintToGameLog(*luaState);
	LuaBindings::Register(*luaState);
}

void ScriptingEngine::Shutdown()
{
	delete luaState;
	luaState = nullptr;
}

sol::state& ScriptingEngine::GetState()
{
	SUNTA_ASSERT(luaState != nullptr, "ScriptingEngine::GetState() called before Init() or after Shutdown()!");
	return *luaState;
}

}