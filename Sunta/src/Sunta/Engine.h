#pragma once
#include <memory>

#include "Renderer.h"
#include "Event.h"
#include "Core.h"

class GLFWwindow;

namespace Sunta
{
	class Scene;
	class Renderer;
	class Event;
	
	class SUNTA_API Engine
	{
	private:
		GLFWwindow*						window;
		Renderer						renderer;
		std::unique_ptr<Scene>			scene;
	
		Event onCloseEvent;
		Event onMenuEvent;
	
		bool isMenuOpen = false;
	
		unsigned int WINDOW_WIDTH	=	1200;
		unsigned int WINDOW_HEIGHT	=	800;
	
		bool Init();
		void ProcessInput();
		void Update(float deltaTime);
		void Render();
		void Shutdown();
	
		void SetCallbacks();
		void SubsribeToEvents();
		void OpenMenu();
	
	public:
		Engine();
		~Engine();
	
		void Run();
	
	};
}