#include "Core/SuntaPreCompiled.h"
#include "SAT.h"

#include <array>
#include <cmath>
#include <glm/glm.hpp>

namespace Sunta
{
// SAT (Separating Axis Theorem)
// 
// DEFINITION:
// 2 convex shapes do NOT overlap if there exists at least one axis onto which,
// after projecting both shapes, their projected intervals ("shadows") don't touch.
// That axis is called a "separating axis".
// 
// IMPLEMENTATION:
// Project both shapes onto a candidate axis -> each becomes a 1D [min, max] range.
// If the two ranges DON'T overlap, that axis proves the shapes aren't touching
// ("separating axis"). If NO candidate axis separates them, the shapes overlap.
//
// HOW IT WORKS FOR OBB VS OBB:
// For two OBBs, only 15 axes can ever possibly be a separating axis:
//   - 3 face-normal axes of box A (local X, Y, Z)
//   - 3 face-normal axes of box B (local X, Y, Z)
//   - 9 cross products of (edge of A) x (edge of B)
// This is a known geometric fact for boxes specifically



// this namespace makes code exists only in SAT.cpp file (INTERNAL LINKAGE, static keyword would work similar)
namespace
{

struct ProjectionInterval
{
	float min;
	float max;
};

inline bool IntervalsOverlap(const ProjectionInterval& a, const ProjectionInterval& b)
{
	return a.min <= b.max && b.min <= a.max;
}

// Projects box onto axis and returns the [min, max] range it covers there
inline ProjectionInterval ProjectOBBOntoAxis(const OBB& box, const glm::vec3& axis)
{
	// box's center projected on axis is the midpoint of the range
	float centerPositionOnAxis = glm::dot(box.center, axis);

	// box is symmetric around its center, so we only need HOW FAR it spreads to each size
	glm::vec3 spreadFromAxisX = box.orientation[0] * box.halfExtents.x;
	glm::vec3 spreadFromAxisY = box.orientation[1] * box.halfExtents.y;
	glm::vec3 spreadFromAxisZ = box.orientation[2] * box.halfExtents.z;

	float halfRangeWidth = std::abs(glm::dot(spreadFromAxisX, axis))
		+ std::abs(glm::dot(spreadFromAxisY, axis))
		+ std::abs(glm::dot(spreadFromAxisZ, axis));

	return { centerPositionOnAxis - halfRangeWidth, centerPositionOnAxis + halfRangeWidth };
}

inline bool IsSeparatingAxis(const OBB& a, const OBB& b, const glm::vec3& axis)
{
	// Edge x edge cross product can come out near-zero for near-parallel edges -
	// that carries no direction info, so treat it as "doesn't separate" and move on
	constexpr float MIN_AXIS_LENGTH_SQUARED = 1e-8f;
	if (glm::dot(axis, axis) < MIN_AXIS_LENGTH_SQUARED)
		return false;

	glm::vec3 normalizedAxis = glm::normalize(axis);
	ProjectionInterval rangeA = ProjectOBBOntoAxis(a, normalizedAxis);
	ProjectionInterval rangeB = ProjectOBBOntoAxis(b, normalizedAxis);

	return !IntervalsOverlap(rangeA, rangeB);
}

constexpr int OBB_AXES_PER_BOX = 3;															   // a box has 3 local axes: X, Y, Z
constexpr int OBB_VS_OBB_FACE_AXIS_COUNT = OBB_AXES_PER_BOX * 2;							   // A 3 axes  +  B 3 axes = 6
constexpr int OBB_VS_OBB_EDGE_AXIS_COUNT = OBB_AXES_PER_BOX * OBB_AXES_PER_BOX;				   // every A axis x every B axis = 9
constexpr int OBB_VS_OBB_AXIS_COUNT = OBB_VS_OBB_FACE_AXIS_COUNT + OBB_VS_OBB_EDGE_AXIS_COUNT; // 6 + 9 = 15

inline std::array<glm::vec3, OBB_VS_OBB_AXIS_COUNT> BuildBoxVsBoxAxes(const OBB& a, const OBB& b)
{
	std::array<glm::vec3, OBB_VS_OBB_AXIS_COUNT> testAxes{};

	// FIRST 3: Box A own face-normal axes
	for (int axis = 0; axis < OBB_AXES_PER_BOX; axis++)
		testAxes[axis] = a.orientation[axis];

	// NEXT 3: Box B own face-normal axes
	for (int axis = 0; axis < OBB_AXES_PER_BOX; axis++)
		testAxes[OBB_AXES_PER_BOX + axis] = b.orientation[axis];

	// LAST 9: Every cross-product combination of (A axis) x (B axis) 
	for (int axisOfA = 0; axisOfA < OBB_AXES_PER_BOX; axisOfA++)
	{
		for (int axisOfB = 0; axisOfB < OBB_AXES_PER_BOX; axisOfB++)
		{
			int slot = OBB_VS_OBB_FACE_AXIS_COUNT + (axisOfA * OBB_AXES_PER_BOX + axisOfB);
			testAxes[slot] = glm::cross(a.orientation[axisOfA], b.orientation[axisOfB]);
		}
	}

	return testAxes;
}

}

// =======================================
//        PUBLIC METHODS BELOW: 
// =======================================

bool Overlaps(const OBB& a, const OBB& b)
{
	std::array<glm::vec3, OBB_VS_OBB_AXIS_COUNT> testAxes = BuildBoxVsBoxAxes(a, b);

	for (const glm::vec3& axis : testAxes)
	{
		// if at least ONE separating axis is found, we're CERTAIN boxes DON'T OVERLAP
		if (IsSeparatingAxis(a, b, axis))
			return false;
	}

	// If none of the 15 candidates axes separated the boxes => THEY OVERLAP
	return true;
}

}