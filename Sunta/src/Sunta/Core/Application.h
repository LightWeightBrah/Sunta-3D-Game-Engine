#pragma once
#include <memory>

#include "Sunta/Renderer/Renderer.h"
#include "Core.h"
#include "Window.h"
#include "Sunta/Events/EventTypes.h"

namespace Sunta
{
	class Scene;
	class Renderer;
	class Event;

	class Application
	{
	public:
		Application();
		virtual ~Application();

		void Run();

	private:
		Renderer						renderer;
		std::unique_ptr<Window>			window;
		std::unique_ptr<Scene>			scene;
	
		EngineMode currentEngineMode = EngineMode::Game;
	
		unsigned int WINDOW_WIDTH	=	1600;
		unsigned int WINDOW_HEIGHT	=	800;
	
		bool isRunning   = true;
		bool isMinimized = false;

		void Init();
		void ProcessInput();
		void Update(float deltaTime);
		void Render();
		void Shutdown();
	
		void SetCallbacks();
		void SubsribeToEvents();
	};

	//THIS SHOULD BE DEFINED BY CLIENT, THAT IS IN ALL GAMES USING SUNTA'S ENGINE
	Application* CreateApplication();
	
}