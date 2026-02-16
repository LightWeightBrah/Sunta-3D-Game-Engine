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
	


	class SUNTA_API Application
	{
	public:
		Application();
		virtual ~Application();

		void Run();

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
	};

	//THIS SHOULD BE DEFINED BY CLIENT, THAT IS IN ALL GAMES USING SUNTA'S ENGINE
	Application* CreateApplication();
	
}