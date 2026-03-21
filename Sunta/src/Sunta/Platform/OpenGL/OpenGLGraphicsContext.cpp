#include "OpenGLGraphicsContext.h"

namespace Sunta
{

OpenGLGraphicsContext::OpenGLGraphicsContext(GLFWwindow* window)
	: window(window)
{

}

void OpenGLGraphicsContext::Configure()
{
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
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