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
		Green
	};

	static void Init();
	static void SetColor(Color color);
	static void Write(const std::string& message);
};

}