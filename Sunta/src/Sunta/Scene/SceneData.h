#pragma once
#include <glm/glm.hpp>

namespace Sunta
{
	class LightSource;
	
	struct LightSourceData
	{
		glm::vec3 position			= glm::vec3(0.0f);

		glm::vec3 ambientIntensity  = glm::vec3(0.2f);
		glm::vec3 diffuseIntensity  = glm::vec3(0.5f);
		glm::vec3 specularIntensity = glm::vec3(1.0f);
	};

	struct SceneData
	{
		glm::mat4 viewMatrix;
		glm::mat4 projectionMatrix;
		glm::vec3 cameraPosition;
	
		LightSourceData lightSourceData;
	};
}