#include "Core/SuntaPreCompiled.h"
#include "LinuxWindow.h"
#include "LinuxOpenGLEditorGUIBackend.h"

#include "Core/Log.h"
#include "Renderer/Renderer.h"
#include "Events/EventBus.h"
#include "Events/EventTypes.h"
#include "Platform/OpenGL/OpenGLGraphicsContext.h"
#include "Renderer/RendererAPI.h"
#include "Core/Assert.h"

#include <GLFW/glfw3.h>

namespace Sunta
{

LinuxWindow::LinuxWindow(const std::string& title, int width, int height)
{
	this->title  = title;
	this->width  = width;
	this->height = height;

	engineModeChangedID = EventBus::Subscribe<EngineModeChangedEvent>([this](const auto& event) { OnEngineModeChanged(event); });

	Init();
}

LinuxWindow::~LinuxWindow()
{
	Shutdown();
}

void LinuxWindow::Init()
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

void LinuxWindow::SetCallbacks()
{
	glfwSetFramebufferSizeCallback(window, [](GLFWwindow* window, int width, int height)
		{
			//we must do this to set new width, height, cause of lambda
			//we cant do here this->width = width
			auto& data = *(LinuxWindow*)glfwGetWindowUserPointer(window);
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

void LinuxWindow::Shutdown()
{
	if (!window)
		return;

	EventBus::Unsubscribe(engineModeChangedID);

	glfwDestroyWindow(window);
	window = nullptr;
	SUNTA_ENGINE_LOG_INFO("Linux Window destroyed on destructor");
}

void LinuxWindow::Update()
{
	graphicsContext->SwapBuffers();
	glfwPollEvents();
}

void LinuxWindow::EnableMouseCursor(bool enabled)
{
	glfwSetInputMode(window, GLFW_CURSOR, enabled ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED);
}

void LinuxWindow::SetAsGraphicsTarget()
{
	graphicsContext->MakeContextCurrent();
}

std::unique_ptr<Sunta::EditorGUIBackend> LinuxWindow::CreateGUIBackend()
{
	return std::make_unique<LinuxOpenGLEditorGUIBackend>();
}

void LinuxWindow::OnEngineModeChanged(const EngineModeChangedEvent& event)
{
	bool shouldShow = (event.mode == EngineMode::Editor);
	EnableMouseCursor(shouldShow);
}


}
