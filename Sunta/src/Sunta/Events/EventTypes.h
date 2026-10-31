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

// The scene open in the editor may have changed: it was edited, saved, loaded or replaced, or Play started / stopped
// Sent through SceneDocument::NotifyChanged(). It doesn't say what changed, whoever needs to know compares the scene
struct SceneChangedEvent
{

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

struct MouseButtonPressedEvent
{
	int button;
};

struct MouseButtonReleasedEvent
{
	int button;
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
