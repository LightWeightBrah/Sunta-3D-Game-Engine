#pragma once
#include <glm/glm.hpp>

#include "Config.h"

struct StaticVertex
{
	glm::vec3 Position;
	glm::vec3 Normal;
	glm::vec2 TexCoords;
};

struct SkinnedVertex : StaticVertex
{
	unsigned int boneIDs[MAX_NUM_BONES_PER_VERTEX];
	float		 weights[MAX_NUM_BONES_PER_VERTEX];
};