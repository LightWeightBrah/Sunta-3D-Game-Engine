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
#include "Editor/EditorGUIContext.h"
#include "Editor/EditorGUI.h"

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
	
			window->Update();

			ProcessInput();
	
			Update(Time::deltaTime);
	
			Render();

			InputManager::Clear();
	
		}
	
		Shutdown();
	}
	
	void Application::Init()
	{
		Log::Init();
		InputManager::Init();

		window = Window::CreateWindow("Sunta Engine", WINDOW_WIDTH, WINDOW_HEIGHT);
		EditorGUIContext::Init(window.get());

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
			isRunning = false;
	
		if (InputManager::IsKeyDown(GLFW_KEY_TAB))
			OpenMenu();
	
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

		EditorGUIContext::NewFrame(window.get());

		EditorGUI::Begin("Sunta Engine Editor");
		EditorGUI::Text("Defualt text");
		EditorGUI::End();

		EditorGUIContext::EndFrame(window.get());
	}
	
	void Application::Shutdown()
	{
		EditorGUIContext::Shutdown(window.get());
		scene->Clear();
	
		if (window)
			window.reset();
	
		glfwTerminate();
	}
	
	void Application::OpenMenu()
	{
		isMenuOpen = !isMenuOpen;
	
		if (isMenuOpen)
		{
			window->EnableMouseCursor(true);
		}
		else
		{
			window->EnableMouseCursor(false);
			InputManager::SetFirstMouse(true);
		}
	}
}
