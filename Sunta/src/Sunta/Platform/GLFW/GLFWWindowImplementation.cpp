#include "Core/SuntaPreCompiled.h"

#include "GLFWWindowImplementation.h"
#include "Core/Log.h"
#include "Renderer/Renderer.h"
#include "Events/EventBus.h"
#include "Events/EventTypes.h"
#include "GLFWOpenGLEditorGUIBackend.h"
#include "Platform/OpenGL/OpenGLGraphicsContext.h"
#include "Renderer/RendererAPI.h"
#include "Core/Assert.h"
#include "Core/EngineAssets.h"

#include <stb/stb_image.h>

#include <GLFW/glfw3.h>

namespace Sunta
{

GLFWWindowImplementation::GLFWWindowImplementation(const std::string& title, int width, int height)
	: title(title)
	, width(width)
	, height(height)
{
	engineModeChangedID = EventBus::Subscribe<EngineModeChangedEvent>([this](const auto& event) { OnEngineModeChanged(event); });
	Init();
}

GLFWWindowImplementation::~GLFWWindowImplementation()
{
	Shutdown();
}

void GLFWWindowImplementation::Init()
{
	if (!glfwInit())
	{
		SUNTA_ENGINE_LOG_ERROR("ERROR: Failed to initialize GLFW");
		return;
	}

	GraphicsContext::Configure();

	glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);

	float xScale = 1.0f, yScale = 1.0f;
	GLFWmonitor* primaryMonitor = glfwGetPrimaryMonitor();

	if (primaryMonitor)
		glfwGetMonitorContentScale(primaryMonitor, &xScale, &yScale);
	else
		SUNTA_ENGINE_LOG_WARNING("Primary monitor not found, using default scale 1.0f");

#if defined(SUNTA_PLATFORM_MAC)
	// Mac instead of pixels uses "points" and "Retina"
	// Logic size in "points", on "Retina" it represents more physcial pixels
	// On Mac, 1 pixel (point) can be 2x2 physical pixels (2.0)
	// Retina is special Mac pixel scale (usually 2x2 for 1 pixel instead of just 1x1 pixel)
	// We use Mac "points" here, GLFW and Mac handles 2x2 pixels automatically
	window = glfwCreateWindow(width, height, title.c_str(), NULL, NULL);
#else
	window = glfwCreateWindow((width * xScale), (height * yScale), title.c_str(), NULL, NULL);
#endif

	if (!window)
	{
		SUNTA_ENGINE_LOG_ERROR("ERROR: Failed to create GLFW window");
		glfwTerminate();
		return;
	}

	SetWindowIcon();

	graphicsContext = GraphicsContext::Create(window);
	graphicsContext->Init();

	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	glfwSetWindowUserPointer(window, this);

	SetCallbacks();
}

void GLFWWindowImplementation::SetCallbacks()
{
	glfwSetFramebufferSizeCallback(window, [](GLFWwindow* window, int width, int height)
		{
			// If we are closing the window don't publish resize events
			if (glfwWindowShouldClose(window))
				return;

			// only publish resize event if window isn't minimized
			if (width <= 0 || height <= 0)
				return;

			// we must do this to set new width, height, cause of lambda
			// we cant do here this->width = width
			auto& data = *(GLFWWindowImplementation*)glfwGetWindowUserPointer(window);
			data.width = width;
			data.height = height;

			EventBus::Publish(WindowResizeEvent{ width, height });
		});

	glfwSetDropCallback(window, [](GLFWwindow*, int count, const char** paths)
		{
			std::vector<std::string> droppedPaths;
			droppedPaths.reserve(count);

			for (unsigned int i = 0; i < count; i++)
			{
				droppedPaths.push_back(paths[i]);
			}

			EventBus::Publish(FileDroppedEvent{ droppedPaths });
		});

	glfwSetCursorPosCallback(window, [](GLFWwindow* window, double xPosition, double yPosition)
		{
			EventBus::Publish(MouseMovedEvent{ static_cast<float>(xPosition), static_cast<float>(yPosition) });
		});

	glfwSetKeyCallback(window, [](GLFWwindow* window, int key, int scancode, int action, int mods)
		{
			switch (action)
			{
			case GLFW_PRESS:   EventBus::Publish(KeyPressedEvent{ key }); break;
			case GLFW_REPEAT:  EventBus::Publish(KeyPressedEvent{ key }); break;
			case GLFW_RELEASE: EventBus::Publish(KeyReleasedEvent{ key }); break;

			}
		});

	glfwSetScrollCallback(window, [](GLFWwindow* window, double xOffset, double yOffset)
		{
			EventBus::Publish(MouseScrollEvent{ static_cast<float>(xOffset), static_cast<float>(yOffset) });
		});

	glfwSetWindowCloseCallback(window, [](GLFWwindow* window)
		{
			glfwHideWindow(window);
			EventBus::Publish(WindowCloseEvent{});

		});
}

void GLFWWindowImplementation::Shutdown()
{
	if (!window)
		return;

	EventBus::Unsubscribe(engineModeChangedID);

	glfwDestroyWindow(window);
	window = nullptr;
	SUNTA_ENGINE_LOG_INFO("GLFW Window destroyed on destructor");
}

void GLFWWindowImplementation::Update()
{
	glfwPollEvents();

	if (glfwWindowShouldClose(window))
		return;

	graphicsContext->SwapBuffers();
}

void GLFWWindowImplementation::EnableMouseCursor(bool enabled)
{
	glfwSetInputMode(window, GLFW_CURSOR, enabled ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED);
}

void GLFWWindowImplementation::SetAsGraphicsTarget()
{
	graphicsContext->MakeContextCurrent();
}


void GLFWWindowImplementation::Show()
{
	glfwShowWindow(window);
}

void GLFWWindowImplementation::OnEngineModeChanged(const EngineModeChangedEvent& event)
{
	bool shouldShow = (event.mode == EngineMode::Editor);
	EnableMouseCursor(shouldShow);
}


void GLFWWindowImplementation::SetWindowIcon()
{
	GLFWimage icon;
	int channels = 0;

	stbi_set_flip_vertically_on_load(false);
	icon.pixels = stbi_load(EngineAssets::App::EngineLogoPath, &icon.width, &icon.height, &channels, 4);

	if (icon.pixels)
	{
		glfwSetWindowIcon(window, 1, &icon);
		stbi_image_free(icon.pixels);
		SUNTA_ENGINE_LOG_INFO("Window icon successfully updated!");
	}
	else
	{
		SUNTA_ENGINE_LOG_WARNING("Failed to load Window icon!");
	}
}

}
