#include <iostream>
#include "GL/glew.h"
#include <GLFW/glfw3.h>

#include <assimp/version.h>

#include "Application.h"
#include "Scene.h"
#include "Renderer.h"
#include "Time.h"
#include "InputManager.h"

#include "Log.h"
#include "EventBus.h"
#include "EventTypes.h"

namespace Sunta
{
	Application::Application()
		: window(nullptr), isMenuOpen(false)
	{
		Init();
	}
	Application::~Application()
	{
	
	}
	
	void Application::Run()
	{
		while (isRunning)
		{
			Time::Update();
	
			ProcessInput();
	
			Update(Time::deltaTime);
	
			Render();
	
			InputManager::Clear();
	
			window->Update();
		}
	
		Shutdown();
	}
	
	bool Application::Init()
	{
		Log::Init();

		window = Window::CreateWindow("Sunta Engine", WINDOW_WIDTH, WINDOW_HEIGHT);

		EventBus::Subscribe<WindowCloseEvent>([this](const auto& event) { isRunning = false; });

		scene = std::make_unique<Scene>();
		scene->Init(window->GetWidth(), window->GetHeight());

		SUNTA_ENGINE_LOG_INFO("Sunta Engine is running!");
	}
	
	void Application::Update(float deltaTime)
	{
		scene->Update();
	}
	
	void Application::ProcessInput()
	{
		if (InputManager::IsKeyDown(GLFW_KEY_ESCAPE))
			onCloseEvent.Invoke();
	
		if (InputManager::IsKeyDown(GLFW_KEY_TAB))
			onMenuEvent.Invoke();
	
		scene->ProcessInput();
	}
	
	void Application::Render()
	{
		renderer.Clear(0.05f, 0.05f, 0.05f, 1.0f);
	
		if (!scene)
		{
			SUNTA_ENGINE_LOG_ERROR("ERROR: Scene is NULL during Render");
			return;
		}
	
		scene->Render(renderer);
	}
	
	void Application::Shutdown()
	{
		scene->Clear();
	
		if (window)
			glfwDestroyWindow(window);
	
		glfwTerminate();
	}
	
	void Application::SubsribeToEvents()
	{
		onCloseEvent.AddListener([this]() { glfwSetWindowShouldClose(window, true);});
		onMenuEvent.AddListener([this]()  { OpenMenu(); });
	}
	
	void Application::OpenMenu()
	{
		isMenuOpen = !isMenuOpen;
	
		if (isMenuOpen)
		{
			glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
		}
		else
		{
			glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
			InputManager::SetFirstMouse(true);
		}
	}
}
