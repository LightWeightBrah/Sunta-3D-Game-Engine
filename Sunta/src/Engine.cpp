#include <iostream>
#include "GL/glew.h"
#include <GLFW/glfw3.h>

#include <assimp/version.h>

#include "Engine.h"
#include "Scene.h"
#include "Renderer.h"
#include "Time.h"
#include "InputManager.h"

Engine::Engine()
	: window(nullptr), isMenuOpen(false)
{

}
Engine::~Engine()
{

}

void Engine::Run()
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

bool Engine::Init()
{
	if (!glfwInit())
	{
		std::cout << "ERROR: Failed to initialize GLFW" << std::endl;
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
		std::cout << "ERROR: Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return false;
	}

	glfwMakeContextCurrent(window);
	
	if (glewInit() != GLEW_OK)
	{
		std::cout << "ERROR: Failed to initalize GLEW" << std::endl;
		return false;
	}

	scene = std::make_unique<Scene>();

	std::cout << glGetString(GL_VERSION) << std::endl;
	
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

void Engine::Update(float deltaTime)
{
	scene->Update();
}

void Engine::ProcessInput()
{
	if (InputManager::IsKeyDown(GLFW_KEY_ESCAPE))
		onCloseEvent.Invoke();

	if (InputManager::IsKeyDown(GLFW_KEY_TAB))
		onMenuEvent.Invoke();

	scene->ProcessInput();
}

void Engine::Render()
{
	renderer.Clear(0.05f, 0.05f, 0.05f, 1.0f);

	if (!scene)
	{
		std::cout << "ERROR: Scene is NULL during Render" << std::endl;
		return;
	}

	scene->Render(renderer);
}

void Engine::Shutdown()
{
	scene->Clear();

	if (window)
		glfwDestroyWindow(window);

	glfwTerminate();
}

void Engine::SetCallbacks()
{
	glfwSetFramebufferSizeCallback(window, [](GLFWwindow* window, int width, int height) 
	{ 
		glViewport(0, 0, width, height);

		Engine* engine = static_cast<Engine*>(glfwGetWindowUserPointer(window));

		if (!(engine && engine->scene))
		{
			std::cout << "ERROR: NO ENGINE OR NO SCENE CREATED" << std::endl;
			return;
		}

		engine->scene->OnWindowResize(static_cast<float>(width), static_cast<float>(height));
	});

	glfwSetCursorPosCallback(window, InputManager::OnMouse);
	glfwSetKeyCallback(window, InputManager::OnSingleKey);
	glfwSetScrollCallback(window, InputManager::OnScroll);
}

void Engine::SubsribeToEvents()
{
	onCloseEvent.AddListener([this]() { glfwSetWindowShouldClose(window, true);});
	onMenuEvent.AddListener([this]()  { OpenMenu(); });
}

void Engine::OpenMenu()
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
