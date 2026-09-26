#pragma once
#include <vector>
#include <functional>
#include <string>

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

struct FileDroppedEvent
{
	std::vector<std::string> paths;
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

struct TriggerEnterEvent
{
	unsigned int triggerEntityID; // entity that has isTrigger = true
	unsigned int otherEntityID;   // entity that entered trigger
};

struct TriggerStayEvent
{
	unsigned int triggerEntityID; // entity that has isTrigger = true
	unsigned int otherEntityID;   // entity that entered trigger
};

struct TriggerExitEvent
{
	unsigned int triggerEntityID; // entity that has isTrigger = true
	unsigned int otherEntityID;   // entity that exited trigger
};

struct CollisionEnterEvent
{
	unsigned int collisionEntityID;
	unsigned int otherEntityID;
};

struct CollisionStayEvent
{
	unsigned int collisionEntityID;
	unsigned int otherEntityID;
};

struct CollisionExitEvent
{
	unsigned int collisionEntityID;
	unsigned int otherEntityID;
};

}
