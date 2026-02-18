#include "../../Terminal.h"
#include "../../PlatformDetection.h"

#ifdef SUNTA_PLATFORM_WINDOWS
#include <windows.h>
#include <iostream>

namespace Sunta {


void Terminal::Init()
{
	HANDLE consoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);
	
	if (consoleHandle == INVALID_HANDLE_VALUE)
		return;

	DWORD currentConsoleMode = 0;

	if (GetConsoleMode(consoleHandle, &currentConsoleMode))
	{
		//ENABLE_VIRTUAL_TERMINAL_PROCESSING (allows to change color via ANSI)
		DWORD colorsEnabledConsoleMode = currentConsoleMode | 0x0004;

		SetConsoleMode(consoleHandle, colorsEnabledConsoleMode);
	}
}

void Terminal::SetColor(Color color)
{
	switch (color)
	{
	case Color::White:		std::cout << "\033[37m"; break;
	case Color::Red:		std::cout << "\033[31m"; break;
	case Color::Yellow:		std::cout << "\033[33m"; break;
	case Color::Green:		std::cout << "\033[32m"; break;
	default:				std::cout << "\033[0m";  break;
	}
}

void Terminal::Write(const std::string& message)
{
	std::cout << message << std::endl;
}

}

#endif