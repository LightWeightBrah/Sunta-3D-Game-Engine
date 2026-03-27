#include "Core/SuntaPreCompiled.h"
#include "Core/Terminal.h"

namespace Sunta 
{

void Terminal::Init()
{
	std::cout << "\033[0m";
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