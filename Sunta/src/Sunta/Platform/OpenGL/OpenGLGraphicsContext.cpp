#include "OpenGLGraphicsContext.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "OpenGLUtilities.h"
#include "Renderer/RendererAPI.h"
#include "Core/Log.h"

namespace Sunta
{

OpenGLGraphicsContext::OpenGLGraphicsContext(GLFWwindow* window)
	: window(window)
{

}

void OpenGLGraphicsContext::Configure()
{
	const auto& config = RendererAPI::GetConfig();

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, config.OpenGLMajor);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, config.OpenGLMinor);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	// We need to add this for GLFW on Mac
#if defined(SUNTA_PLATFORM_MACOS)
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

	SUNTA_ENGINE_LOG_INFO("Started with OpenGL: {0}.{1}", config.OpenGLMajor, config.OpenGLMinor);
}

void OpenGLGraphicsContext::Init()
{
	glfwMakeContextCurrent(window);

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		SUNTA_ENGINE_LOG_ERROR("ERROR: Failed to initalize GLAD");
		return;
	}

	SUNTA_ENGINE_LOG_INFO("OpenGL Version: {}", glGetString(GL_VERSION));

	GLCall(glEnable(GL_DEPTH_TEST));
}

void OpenGLGraphicsContext::SwapBuffers()
{
	glfwSwapBuffers(window);
}

void OpenGLGraphicsContext::MakeContextCurrent()
{
	glfwMakeContextCurrent(window);
}

}