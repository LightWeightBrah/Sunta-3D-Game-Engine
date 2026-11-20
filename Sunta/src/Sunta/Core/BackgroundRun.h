#pragma once

#include <cstdlib>

namespace Sunta
{

// Automated test runs can set SUNTA_BACKGROUND=1: the game runs and draws as usual (input scripts, screenshots),
// but its window is never shown, it doesn't go fullscreen and no console window opens, so nothing pops up
// over what the person at the computer is doing
inline bool IsBackgroundRun()
{
	static const bool isBackground = []
	{
		const char* value = std::getenv("SUNTA_BACKGROUND");
		return value && value[0] != 0 && value[0] != '0';
	}();
	return isBackground;
}

}
