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

		// Recording a test (SUNTA_FIXED_DT=0.033): every frame is the same step of game time however long it took to
		// draw and save, so frames saved one after another make a smooth video. Sounds and music keep real time
		static const float fixedStep = []
		{
			const char* value = std::getenv("SUNTA_FIXED_DT");
			try { return (value && *value) ? std::stof(value) : 0.0f; }
			catch (...) { return 0.0f; }
		}();
		if (fixedStep > 0.0f)
			deltaTime = fixedStep;
	}
}