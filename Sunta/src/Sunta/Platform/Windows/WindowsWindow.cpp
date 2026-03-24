#include "Core/SuntaPreCompiled.h"

#include "WindowsWindow.h"
#include "Core/Log.h"
#include "Renderer/Renderer.h"
#include "Events/EventBus.h"
#include "Events/EventTypes.h"
#include "EditorGUIBackendWindows.h"
#include "Platform/OpenGL/OpenGLGraphicsContext.h"
#include "Renderer/RendererAPI.h"
#include "Core/Assert.h"

#include <GLFW/glfw3.h>

namespace Sunta
{

WindowsWindow::WindowsWindow(int width, int height, const std::string& title)
{
	this->width  = width;
	this->height = height;
	this->title  = title;

	engineModeChangedID = EventBus::Subscribe<EngineModeChangedEvent>([this](const auto& event) { OnEngineModeChanged(event); });

	Init();
}

WindowsWindow::~WindowsWindow()
{
	if (!window)
		return;
	
	EventBus::Unsubsribe(engineModeChangedID);

	glfwDestroyWindow(window);
	window = nullptr;
	SUNTA_ENGINE_LOG_INFO("Windows Window destroyed on destructor");
}


std::unique_ptr<Window> Window::CreateWindow(const std::string& title, int width, int height)
{
	return std::make_unique<WindowsWindow>(width, height, title);
}

void WindowsWindow::Init()
{
	if (!glfwInit())
	{
		SUNTA_ENGINE_LOG_ERROR("ERROR: Failed to initialize GLFW");
		return;
	}

	GraphicsContext::Configure();

	float xScale = 1.0f, yScale = 1.0f;
	GLFWmonitor* primaryMonitor = glfwGetPrimaryMonitor();

	if (primaryMonitor)
		glfwGetMonitorContentScale(primaryMonitor, &xScale, &yScale);
	else
		SUNTA_ENGINE_LOG_WARNING("Primary monitor not found, using default scale 1.0f");

	window = glfwCreateWindow((width * xScale), (height * yScale), title.c_str(), NULL, NULL);

	if (!window)
	{
		SUNTA_ENGINE_LOG_ERROR("ERROR: Failed to create GLFW window");
		glfwTerminate();
		return;
	}

	graphicsContext = GraphicsContext::Create(window);
	graphicsContext->Init();

	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	glfwSetWindowUserPointer(window, this);

	SetCallbacks();
}

void WindowsWindow::SetCallbacks()
{
	glfwSetFramebufferSizeCallback(window, [](GLFWwindow* window, int width, int height)
		{
			//we must do this to set new width, height, cause of lambda
			//we cant do here this->width = width
			auto& data = *(WindowsWindow*)glfwGetWindowUserPointer(window);
			data.width = width;
			data.height = height;

			EventBus::Publish(WindowResizeEvent{ width, height });

		});

	glfwSetCursorPosCallback(window, [](GLFWwindow* window, double xPosition, double yPosition)
		{
			EventBus::Publish(MouseMovedEvent{ static_cast<float>(xPosition), static_cast<float>(yPosition)});
		});

	glfwSetKeyCallback(window, [](GLFWwindow* window, int key, int scancode, int action, int mods)
		{
			switch (action)
			{
			case GLFW_PRESS :   EventBus::Publish(KeyPressedEvent{ key }); break;
			case GLFW_REPEAT :  EventBus::Publish(KeyPressedEvent{ key }); break;
			case GLFW_RELEASE : EventBus::Publish(KeyReleasedEvent{ key }); break;

			}
		});

	glfwSetScrollCallback(window, [](GLFWwindow* window, double xOffset, double yOffset)
		{
			EventBus::Publish(MouseScrollEvent{ static_cast<float>(xOffset), static_cast<float>(yOffset)});
		});

	glfwSetWindowCloseCallback(window, [](GLFWwindow* window)
		{
			EventBus::Publish(WindowCloseEvent{});

		});
}

void WindowsWindow::Update()
{
	graphicsContext->SwapBuffers();
	glfwPollEvents();
}

void WindowsWindow::EnableMouseCursor(bool enabled)
{
	glfwSetInputMode(window, GLFW_CURSOR, enabled ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED);
}

void WindowsWindow::SetAsGraphicsTarget()
{
	graphicsContext->MakeContextCurrent();
}


std::unique_ptr<Sunta::EditorGUIBackend> WindowsWindow::CreateGUIBackend()
{
	return std::make_unique<EditorGUIBackendWindows>();
}

void WindowsWindow::OnEngineModeChanged(const EngineModeChangedEvent& event)
{
	bool shouldShow = (event.mode == EngineMode::Editor);
	EnableMouseCursor(shouldShow);
}


}
