#include "SuntaPreCompiled.h"

#include <GLFW/glfw3.h>

#include "EngineTime.h"

namespace Sunta
{
	float EngineTime::deltaTime = 0.0f;
	float EngineTime::lastFrame = 0.0f;
	
	void EngineTime::Update()
	{
		float currentFrame = static_cast<float>(glfwGetTime());
	
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;
	}
}