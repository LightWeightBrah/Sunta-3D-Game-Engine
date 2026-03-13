#pragma once
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include "Events/EventTypes.h"

namespace Sunta
{
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
		static void Init();

		static void OnKeyPressed (const KeyPressedEvent&  event);
		static void OnKeyReleased(const KeyReleasedEvent& event);
		static void OnMouseMoved (const MouseMovedEvent&  event);
		static void OnMouseScroll(const MouseScrollEvent& event);
	
		static bool IsKeyPressed (int keyCode);
		static bool IsKeyDown	 (int keyCode);
	
		static void Clear();
	
		static glm::vec2 GetMousePosition()		{ return mousePosition; }
		static glm::vec2 GetMouseDelta()		{ return mouseDelta;	}
		static float	 GetScrollOffset()		{ return scrollOffset;	}
		
		static void ResetScroll()				{ scrollOffset = 0.0f;	}
	
		static void SetFirstMouse(bool value)	{ firstMouse = value;   }
	};
}