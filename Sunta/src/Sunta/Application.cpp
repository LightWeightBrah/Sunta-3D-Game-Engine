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

namespace Sunta
{
	Application::Application()
		: window(nullptr), isMenuOpen(false)
	{
		Log::Init();

		SUNTA_ENGINE_LOG_INFO("Sunta Engine is running!");
	
	}
	Application::~Application()
	{
	
	}
	
	void Application::Run()
	{
		if (!Init())
			return;
	
		


		while (!glfwWindowShouldClose(window))
		{
			Time::Update();
	
			ProcessInput();
	
			Update(Time::deltaTime);
	
			Render();
	
			InputManager::Clear();
	
			glfwSwapBuffers(window);
			glfwPollEvents();
		}
	
		Shutdown();
	}
	
	bool Application::Init()
	{
		if (!glfwInit())
		{
			SUNTA_ENGINE_LOG_ERROR("ERROR: Failed to initialize GLFW");
			return false;
		}
	
		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
		glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	
		float xScale, yScale;
		glfwGetMonitorContentScale(glfwGetPrimaryMonitor(), &xScale, &yScale);
	
		window = glfwCreateWindow(
			static_cast<int>((WINDOW_WIDTH  * xScale)),
			static_cast<int>((WINDOW_HEIGHT * yScale)),
			"SunFinder", NULL, NULL);
	
		if (!window)
		{
			SUNTA_ENGINE_LOG_ERROR("ERROR: Failed to create GLFW window");
			glfwTerminate();
			return false;
		}
	
		glfwMakeContextCurrent(window);
		
		if (glewInit() != GLEW_OK)
		{
			SUNTA_ENGINE_LOG_ERROR("ERROR: Failed to initalize GLEW");
			return false;
		}
	
		scene = std::make_unique<Scene>();
	
		SUNTA_ENGINE_LOG_INFO("{}", glGetString(GL_VERSION));
		
		GLCall(glEnable(GL_DEPTH_TEST));
		glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
		glfwSetWindowUserPointer(window, this);
	
		SetCallbacks();
		SubsribeToEvents();
		
		int width, height;
		glfwGetFramebufferSize(window, &width, &height);
		scene->Init(static_cast<float>(width), static_cast<float>(height));
	
		return true;
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
	
	void Application::SetCallbacks()
	{
		glfwSetFramebufferSizeCallback(window, [](GLFWwindow* window, int width, int height) 
		{ 
			glViewport(0, 0, width, height);
	
			Application* engine = static_cast<Application*>(glfwGetWindowUserPointer(window));
	
			if (!(engine && engine->scene))
			{
				SUNTA_ENGINE_LOG_ERROR("ERROR: NO ENGINE OR NO SCENE CREATED");
				return;
			}
	
			engine->scene->OnWindowResize(static_cast<float>(width), static_cast<float>(height));
		});
	
		glfwSetCursorPosCallback(window, InputManager::OnMouse);
		glfwSetKeyCallback(window, InputManager::OnSingleKey);
		glfwSetScrollCallback(window, InputManager::OnScroll);
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
