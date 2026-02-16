#include <GLFW/glfw3.h>

#include "Time.h"

float Time::deltaTime = 0.0f;
float Time::lastFrame = 0.0f;

void Time::Update()
{
	float currentFrame = static_cast<float>(glfwGetTime());

	deltaTime = currentFrame - lastFrame;
	lastFrame = currentFrame;
}