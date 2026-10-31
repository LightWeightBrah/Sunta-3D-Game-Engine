#include "SuntaPreCompiled.h"

#include "InputManager.h"
#include "Events/EventBus.h"
#include "Events/EventTypes.h"

namespace Sunta {

bool		InputManager::keysPressed[1024]		= { false };
bool		InputManager::keysChanged[1024]		= { false };

bool		InputManager::mouseButtonsPressed[MouseButtonCount] = { false };
bool		InputManager::mouseButtonsChanged[MouseButtonCount] = { false };
	
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
	EventBus::Subscribe<MouseButtonPressedEvent>(InputManager::OnMouseButtonPressed);
	EventBus::Subscribe<MouseButtonReleasedEvent>(InputManager::OnMouseButtonReleased);
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
		return; //makes sure mouseDelta is always 0 when received multiple mouse moved events in 1 frame 
	}
	
	mouseDelta.x += x - lastMousePosition.x;
	mouseDelta.y += lastMousePosition.y - y;
	
	lastMousePosition	= { x, y };
	mousePosition		= { x, y };
}
	
void InputManager::OnMouseScroll(const MouseScrollEvent& event)
{
	scrollOffset = event.yOffset;
}

void InputManager::OnMouseButtonPressed(const MouseButtonPressedEvent& event)
{
	if (event.button >= 0 && event.button < MouseButtonCount)
	{
		mouseButtonsPressed[event.button] = true;
		mouseButtonsChanged[event.button] = true;
	}
}

void InputManager::OnMouseButtonReleased(const MouseButtonReleasedEvent& event)
{
	if (event.button >= 0 && event.button < MouseButtonCount)
		mouseButtonsPressed[event.button] = false;
}
	
bool InputManager::IsKeyPressed(int keyCode)
{
	return keysPressed[keyCode];
}
	
bool InputManager::IsKeyDown(int keyCode)
{
	return keysPressed[keyCode] && keysChanged[keyCode];
}

bool InputManager::IsMouseButtonPressed(int button)
{
	return button >= 0 && button < MouseButtonCount && mouseButtonsPressed[button];
}

bool InputManager::IsMouseButtonDown(int button)
{
	// Not "pressed and still held": a quick click can be pressed and released within the same frame
	return button >= 0 && button < MouseButtonCount && mouseButtonsChanged[button];
}
	
void InputManager::Clear()
{
	std::memset(keysChanged, 0, sizeof(keysChanged));
	std::memset(mouseButtonsChanged, 0, sizeof(mouseButtonsChanged));
	
	mouseDelta		= { 0.0f, 0.0f };
	scrollOffset	= 0.0f;
}

}