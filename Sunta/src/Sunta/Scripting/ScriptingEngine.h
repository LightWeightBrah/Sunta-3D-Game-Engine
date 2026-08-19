#pragma once

#include <sol/sol.hpp>
#include <string>

namespace Sunta
{

class ScriptingEngine
{
public:
	static void Init();
	static void Shutdown();

	static sol::state& GetState();

private:
	inline static sol::state* luaState = nullptr;
};


}