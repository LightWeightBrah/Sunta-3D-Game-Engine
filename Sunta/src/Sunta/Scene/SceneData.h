#pragma once
#include <glm/glm.hpp>

#include "Sunta/Renderer/LightingCommon.h"

namespace Sunta
{
	
class LightSource;
	
	
struct DirectionalLightData
{
	glm::vec3	direction			= glm::vec3(0.0f);
	
	LightColor	color;
};

struct PointLightData
{
	glm::vec3	position			= glm::vec3(0.0f);

	LightColor	color;
	Attenuation attenuation;
};

struct SpotlightData
{
	glm::vec3	position			= glm::vec3(0.0f);
	glm::vec3	spotlightDirection	= glm::vec3(0.0f);

	float		innercutOffAngle	= 12.5f;
	float		outerCutOffAngle	= 17.5f;

	LightColor	color;
	Attenuation attenuation;
};

struct SceneData
{
	glm::mat4 viewMatrix;
	glm::mat4 projectionMatrix;
	glm::vec3 cameraPosition;

	// Usually we only have 1 directional light (sun), but let's keep it flexible for the user
	std::vector<DirectionalLightData>	directionalLights;
	std::vector<PointLightData>			pointLights;
	std::vector<SpotlightData>			spotlights;
};

}