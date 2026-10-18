#pragma once
#include <string>

namespace Sunta 
{

class Terminal
{
public:
	enum class Color
	{
		White = 0,
		Red,
		Yellow,
		Green,
		Blue
	};

	static void Init();

	// A game without a console window (a Windows Release build) gets one, so its log can be read live
	// Does nothing where a console already exists or where the system has no such notion. Returns true if a console is available
	static bool OpenConsole();
	static void SetColor(Color color);
	static void Write(const std::string& message);
};

}