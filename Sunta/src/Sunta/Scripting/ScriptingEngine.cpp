#include "Core/SuntaPreCompiled.h"
#include "ScriptingEngine.h"
#include "Core/Assert.h"
#include "LuaBindings.h"

namespace Sunta
{

void ScriptingEngine::Init()
{
	luaState = new sol::state();
	luaState->open_libraries(sol::lib::base, sol::lib::math, sol::lib::table, sol::lib::os, sol::lib::string);
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