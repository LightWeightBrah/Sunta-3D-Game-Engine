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

	// Distance fog (from the scene's Environment): far things fade into 'fogColor'. Density 0 = no fog
	glm::vec3 fogColor   = glm::vec3(0.0f);
	float     fogDensity = 0.0f;

	// The shadows of the first directional light (filled by ShadowRenderer when its shadow map was drawn this frame)
	bool      hasShadows         = false;
	glm::mat4 lightSpaceMatrix   = glm::mat4(1.0f);
	float     shadowStrength     = 0.0f;
	float     shadowNormalOffset = 0.0f;
	int       shadowSoftness     = 1;
};

}