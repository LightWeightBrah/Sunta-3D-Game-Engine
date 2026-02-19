#include "GL/glew.h"
#include <GLFW/glfw3.h>

#include "WindowsWindow.h"
#include "../../Log.h"
#include "../../Renderer.h"
#include "../../EventBus.h"
#include "../../EventTypes.h"
#include "EditorGUIBackendWindows.h"

namespace Sunta
{

WindowsWindow::WindowsWindow(int width, int height, const std::string& title)
{
	this->width  = width;
	this->height = height;
	this->title  = title;

	Init();
}

WindowsWindow::~WindowsWindow()
{
	if (!window)
		return;
	
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

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	float xScale, yScale;
	glfwGetMonitorContentScale(glfwGetPrimaryMonitor(), &xScale, &yScale);

	window = glfwCreateWindow((width * xScale), (height * yScale), title.c_str(), NULL, NULL);

	if (!window)
	{
		SUNTA_ENGINE_LOG_ERROR("ERROR: Failed to create GLFW window");
		glfwTerminate();
		return;
	}

	glfwMakeContextCurrent(window);

	if (glewInit() != GLEW_OK)
	{
		SUNTA_ENGINE_LOG_ERROR("ERROR: Failed to initalize GLEW");
		return;
	}

	SUNTA_ENGINE_LOG_INFO("{}", glGetString(GL_VERSION));

	GLCall(glEnable(GL_DEPTH_TEST));
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	glfwSetWindowUserPointer(window, this);

	SetCallbacks();
}

void WindowsWindow::SetCallbacks()
{
	glfwSetFramebufferSizeCallback(window, [](GLFWwindow* window, int width, int height)
		{
			glViewport(0, 0, width, height);

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
	glfwSwapBuffers(window);
	glfwPollEvents();
}

void WindowsWindow::EnableMouseCursor(bool enabled)
{
	glfwSetInputMode(window, GLFW_CURSOR, enabled ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED);
}

void WindowsWindow::SetAsGraphicsTarget()
{
	glfwMakeContextCurrent(window);
}


std::unique_ptr<Sunta::EditorGUIBackend> WindowsWindow::CreateGUIBackend()
{
	return std::make_unique<EditorGUIBackendWindows>();
}

}
