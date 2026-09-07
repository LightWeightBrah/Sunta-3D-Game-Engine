#pragma once

#include <array>
#include <glm/glm.hpp>

namespace Sunta
{

// Oriented Bounding Box (Similar To AABB (Axis-Aligned-Bounding-Box, but OBB can be rotated)
struct OBB
{
	glm::vec3 center      = glm::vec3(0.0f);
	glm::vec3 halfExtents = glm::vec3(0.5f);
	glm::mat3 orientation = glm::mat3(1.0f);
};

// pair (A,B) is same as pair (B,A) (The Order doesn't matter)
struct OverlapPair
{
	unsigned int entityA;
	unsigned int entityB;

	bool operator==(const OverlapPair& other) const
	{
		return(entityA == other.entityA && entityB == other.entityB)
		   || (entityA == other.entityB && entityB == other.entityA);
	}
};

inline OBB MakeWorldOBB(const glm::mat4& worldMatrix, const glm::vec3& localOffset, const glm::vec3& localHalfExtents)
{
	glm::vec3 axisX = glm::vec3(worldMatrix[0]);
	glm::vec3 axisY = glm::vec3(worldMatrix[1]);
	glm::vec3 axisZ = glm::vec3(worldMatrix[2]);

	glm::vec3 scale = glm::vec3(glm::length(axisX), glm::length(axisY), glm::length(axisZ));

	OBB obb;

	glm::vec3 normalizedX = (scale.x > 0.0f) ? axisX / scale.x : glm::vec3(1.0f, 0.0f, 0.0f);
	glm::vec3 normalizedY = (scale.y > 0.0f) ? axisY / scale.y : glm::vec3(0.0f, 1.0f, 0.0f);
	glm::vec3 normalizedZ = (scale.z > 0.0f) ? axisZ / scale.z : glm::vec3(0.0f, 0.0f, 1.0f);

	obb.orientation = glm::mat3(normalizedX, normalizedY, normalizedZ);

	obb.halfExtents = localHalfExtents * scale;

	glm::vec3 scaledOffset        = localOffset * scale;
	glm::vec3 rotatedOffset       = obb.orientation * scaledOffset;
	glm::vec3 entityWorldPosition = glm::vec3(worldMatrix[3]);

	obb.center = entityWorldPosition + rotatedOffset;

	return obb;
}

constexpr unsigned int BOX_CORNER_COUNT = 8;
const glm::vec3 BOX_CORNER_SIGNS[BOX_CORNER_COUNT] = 
{
	//  X      Y      Z
	{ -1.0f, -1.0f, -1.0f }, // 0: left   bottom  back  
	{ +1.0f, -1.0f, -1.0f }, // 1: right  bottom  back  
	{ +1.0f, -1.0f, +1.0f }, // 2: right  bottom  front 
	{ -1.0f, -1.0f, +1.0f }, // 3: left   bottom  front 
	{ -1.0f, +1.0f, -1.0f }, // 4: left   top     back  
	{ +1.0f, +1.0f, -1.0f }, // 5: right  top     back  
	{ +1.0f, +1.0f, +1.0f }, // 6: right  top     front 
	{ -1.0f, +1.0f, +1.0f }  // 7: left   top     front 
};

inline std::array<glm::vec3, BOX_CORNER_COUNT> GetOBBCorners(const OBB& box)
{
	std::array<glm::vec3, BOX_CORNER_COUNT> corners{};

	for (unsigned int i = 0; i < BOX_CORNER_COUNT; i++)
	{
		glm::vec3 localCorner = BOX_CORNER_SIGNS[i] * box.halfExtents;

		corners[i] = box.center + box.orientation * localCorner;
	}

	return corners;
}


}