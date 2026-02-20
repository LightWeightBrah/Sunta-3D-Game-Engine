#pragma once
#include <vector>
#include <functional>

namespace Sunta
{

enum class EngineMode
{
	Game,
	Editor
};

struct EngineModeChangedEvent
{
	EngineMode mode;
};

struct WindowResizeEvent
{
	int width, height;
};

struct WindowCloseEvent
{

};

struct MouseMovedEvent
{
	float xPosition, yPosition;
};

struct MouseScrollEvent
{
	float xOffset, yOffset;
};

struct KeyPressedEvent
{
	int keyCode;
};

struct KeyReleasedEvent
{
	int keyCode;
};

}
