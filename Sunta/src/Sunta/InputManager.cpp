#include <cstring>

#include "InputManager.h"
#include "EventBus.h"
#include "EventTypes.h"

namespace Sunta {

bool		InputManager::keysPressed[1024]		= { false };
bool		InputManager::keysChanged[1024]		= { false };
	
glm::vec2	InputManager::mousePosition			= { 0.0f, 0.0f };
glm::vec2	InputManager::lastMousePosition		= { 0.0f, 0.0f };
glm::vec2	InputManager::mouseDelta			= { 0.0f, 0.0f };
bool		InputManager::firstMouse			= true;
	
float		InputManager::scrollOffset			= 0.0f;
	
void InputManager::Init()
{
	EventBus::Subscribe<KeyPressedEvent>(InputManager::OnKeyPressed);
	EventBus::Subscribe<KeyReleasedEvent>(InputManager::OnKeyReleased);
	EventBus::Subscribe<MouseMovedEvent>(InputManager::OnMouseMoved);
	EventBus::Subscribe<MouseScrollEvent>(InputManager::OnMouseScroll);
}

void InputManager::OnKeyPressed(const KeyPressedEvent& event)
{
	if (event.keyCode >= 0 && event.keyCode < 1024)
	{
		keysPressed[event.keyCode] = true;
		keysChanged[event.keyCode] = true;
	}
}

void InputManager::OnKeyReleased(const KeyReleasedEvent& event)
{
	if (event.keyCode >= 0 && event.keyCode < 1024)
	{
		keysPressed[event.keyCode] = false;
		keysChanged[event.keyCode] = false;
	}
}
	
void InputManager::OnMouseMoved(const MouseMovedEvent& event)
{
	float x = event.xPosition;
	float y = event.yPosition;
	
	if (firstMouse)
	{
		lastMousePosition = { x, y };
		firstMouse = false;
	}
	
	mouseDelta.x = x - lastMousePosition.x;
	mouseDelta.y = lastMousePosition.y - y;
	
	lastMousePosition	= { x, y };
	mousePosition		= { x, y };
}
	
void InputManager::OnMouseScroll(const MouseScrollEvent& event)
{
	scrollOffset = event.yOffset;
}
	
bool InputManager::IsKeyPressed(int keyCode)
{
	return keysPressed[keyCode];
}
	
bool InputManager::IsKeyDown(int keyCode)
{
	return keysPressed[keyCode] && keysChanged[keyCode];
}
	
void InputManager::Clear()
{
	std::memset(keysChanged, 0, sizeof(keysChanged));
	
	mouseDelta		= { 0.0f, 0.0f };
	scrollOffset	= 0.0f;
}

}