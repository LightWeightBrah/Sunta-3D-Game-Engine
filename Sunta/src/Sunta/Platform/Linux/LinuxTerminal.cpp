#include "Core/SuntaPreCompiled.h"
#include "Core/Terminal.h"

namespace Sunta 
{

void Terminal::Init()
{
	std::cout << "\033[0m";
}

bool Terminal::OpenConsole()
{
	// A game started from a terminal prints there, one started from a file manager has no terminal to open
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