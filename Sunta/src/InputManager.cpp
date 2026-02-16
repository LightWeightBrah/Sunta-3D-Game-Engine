#include <cstring>

#include "InputManager.h"

bool		InputManager::keysPressed[1024]		= { false };
bool		InputManager::keysChanged[1024]		= { false };

glm::vec2	InputManager::mousePosition			= { 0.0f, 0.0f };
glm::vec2	InputManager::lastMousePosition		= { 0.0f, 0.0f };
glm::vec2	InputManager::mouseDelta			= { 0.0f, 0.0f };
bool		InputManager::firstMouse			= true;

float		InputManager::scrollOffset			= 0.0f;

void InputManager::OnSingleKey(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (key >= 0 && key < 1024)
	{
		if (action == GLFW_PRESS)
		{
			keysPressed[key]			= true;
			keysChanged[key]			= true;
		}
		else if (action == GLFW_RELEASE)
		{
			keysPressed[key]			= false;
			keysChanged[key]			= false;
		}
	}
}

void InputManager::OnMouse(GLFWwindow* window, double xPos, double yPos)
{
	float x = static_cast<float>(xPos);
	float y = static_cast<float>(yPos);

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

void InputManager::OnScroll(GLFWwindow* window, double xOffset, double yOffset)
{
	scrollOffset = static_cast<float>(yOffset);
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