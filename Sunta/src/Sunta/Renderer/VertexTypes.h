#pragma once
#include <glm/glm.hpp>

#include "Core/Config.h"

namespace Sunta
{
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
		// Where its expression shape (the mesh's first morph target) moves this vertex: a smile, a frown
		glm::vec3	 expressionOffset;
	};
}