#include "Core/SuntaPreCompiled.h"
#include "MacWindow.h"
#include "MacOpenGLEditorGUIBackend.h"

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

MacWindow::MacWindow(const std::string& title, int width, int height)
{
	// Mac instead of pixels uses "points" and "Retina"
	this->title  = title; // Logic size in "points", on "Retina" it represents more physcial pixels
	this->width  = width; // On Mac, 1 pixel (point) can be 2x2 physical pixels (2.0)
	this->height = height;

	engineModeChangedID = EventBus::Subscribe<EngineModeChangedEvent>([this](const auto& event) { OnEngineModeChanged(event); });

	Init();
}

MacWindow::~MacWindow()
{
	Shutdown();
}

void MacWindow::Init()
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
	{
		glfwGetMonitorContentScale(primaryMonitor, &xScale, &yScale);
		SUNTA_ENGINE_LOG_INFO("Mac Retina Scale: {0}", xScale); // retina is special Mac pixel scale (usually 2x2 for 1 pixel instead of just 1x1 pixel)
		// Log is for UI scaling (ImGUI), NOT window creation

	}
	else
	{
		SUNTA_ENGINE_LOG_WARNING("Mac Primary monitor not found!");
	}

	// we use Mac "points" here, GLFW and Mac handles 2x2 pixels automatically
	window = glfwCreateWindow(width, height, title.c_str(), NULL, NULL);

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

void MacWindow::SetCallbacks()
{
	glfwSetFramebufferSizeCallback(window, [](GLFWwindow* window, int width, int height)
		{
			//we must do this to set new width, height, cause of lambda
			//we cant do here this->width = width
			auto& data = *(MacWindow*)glfwGetWindowUserPointer(window);
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

void MacWindow::Shutdown()
{
	if (!window)
		return;

	EventBus::Unsubsribe(engineModeChangedID);

	glfwDestroyWindow(window);
	window = nullptr;
	SUNTA_ENGINE_LOG_INFO("Mac Window destroyed on destructor");
}

void MacWindow::Update()
{
	graphicsContext->SwapBuffers();
	glfwPollEvents();
}

void MacWindow::EnableMouseCursor(bool enabled)
{
	glfwSetInputMode(window, GLFW_CURSOR, enabled ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED);
}

void MacWindow::SetAsGraphicsTarget()
{
	graphicsContext->MakeContextCurrent();
}

std::unique_ptr<Sunta::EditorGUIBackend> MacWindow::CreateGUIBackend()
{
	return std::make_unique<MacOpenGLEditorGUIBackend>();
}

void MacWindow::OnEngineModeChanged(const EngineModeChangedEvent& event)
{
	bool shouldShow = (event.mode == EngineMode::Editor);
	EnableMouseCursor(shouldShow);
}


}
