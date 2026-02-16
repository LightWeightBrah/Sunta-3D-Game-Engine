#pragma once
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

class InputManager
{
private:
	static bool			keysPressed[1024];
	static bool			keysChanged[1024];

	static glm::vec2	mousePosition;
	static glm::vec2	lastMousePosition;
	static glm::vec2	mouseDelta;
	static bool			firstMouse;

	static float		scrollOffset;

public:
	static void OnSingleKey(GLFWwindow* window, int key, int scancode, int action, int mods);
	static void OnMouse    (GLFWwindow* window, double xPos,    double yPos);
	static void OnScroll   (GLFWwindow* window, double xOffset, double yOffset);

	static bool IsKeyPressed(int keyCode);
	static bool IsKeyDown	(int keyCode);

	static void Clear();

	static glm::vec2	GetMousePosition()	{ return mousePosition; }
	static glm::vec2	GetMouseDelta()		{ return mouseDelta;	}
	static float		GetScrollOffset()	{ return scrollOffset;	}
	
	static void			ResetScroll()		{ scrollOffset = 0.0f;	}

	static void			SetFirstMouse(bool value)	{ firstMouse = value; }
};