#pragma once
#include "src/Sunta/Window.h"

namespace Sunta
{

class EditorGUIContext
{
public:
	static void Init(Window* window);
	static void Shutdown();

	static void NewFrame();
	static void EndFrame();
};

}