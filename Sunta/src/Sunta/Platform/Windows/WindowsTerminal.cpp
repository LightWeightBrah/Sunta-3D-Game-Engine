#include "Core/SuntaPreCompiled.h"
#include "Core/Terminal.h"

namespace Sunta 
{

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

bool Terminal::OpenConsole()
{
	// Debug builds (and anything started from a console) already have one
	if (GetConsoleWindow() != nullptr)
		return true;

	if (!AllocConsole())
		return false;

	// The standard streams were closed at start (the program is a Windows application), now they point to the new console
	FILE* stream = nullptr;
	freopen_s(&stream, "CONOUT$", "w", stdout);
	freopen_s(&stream, "CONOUT$", "w", stderr);
	std::cout.clear();
	std::cerr.clear();

	Init();
	return true;
}

void Terminal::SetColor(Color color)
{
	switch (color)
	{
	case Color::White:		std::cout << "\033[37m"; break;
	case Color::Red:		std::cout << "\033[31m"; break;
	case Color::Yellow:		std::cout << "\033[33m"; break;
	case Color::Green:		std::cout << "\033[32m"; break;
	case Color::Blue:		std::cout << "\033[94m"; break; // the bright blue, the normal one is hard to read on black
	default:				std::cout << "\033[0m";  break;
	}
}

void Terminal::Write(const std::string& message)
{
	std::cout << message << std::endl;
}

}