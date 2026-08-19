#include "Core/SuntaPreCompiled.h"
#include "ScriptingEngine.h"

namespace Sunta
{

void ScriptingEngine::Init()
{
	luaState = new sol::state();
	luaState->open_libraries(sol::lib::base, sol::lib::math, sol::lib::table, sol::lib::os);
}

void ScriptingEngine::Shutdown()
{
	delete luaState;
	luaState = nullptr;
}

}