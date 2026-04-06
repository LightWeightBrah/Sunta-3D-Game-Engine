#pragma once

#include <glm/glm.hpp>

namespace Sunta
{

struct LightColor
{
	glm::vec3 ambientIntensity	= glm::vec3(0.2f);
	glm::vec3 diffuseIntensity	= glm::vec3(0.5f);
	glm::vec3 specularIntensity = glm::vec3(1.0f);
};

struct Attenuation
{
	float constant	= 1.0f;
	float linear	= 0.09f;
	float quadratic = 0.032f;
};

}