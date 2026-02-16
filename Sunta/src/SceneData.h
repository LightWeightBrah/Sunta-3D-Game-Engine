#pragma once
#include <glm/glm.hpp>

class LightSource;

struct SceneData
{
	glm::mat4 viewMatrix;
	glm::mat4 projectionMatrix;
	glm::vec3 cameraPosition;

	LightSource* lightSource = nullptr;
};