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

// Raycasting (for editor entity picking)
struct Ray
{
	glm::vec3 origin    = glm::vec3(0.0f);
	glm::vec3 direction = glm::vec3(0.0f, 0.0f, -1.0f); // this should be already normalized
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

// Transforms a world-space ray into the OBB's local space
// In this local space, the box is centered at the origin (0,0,0) and is axis-aligned (AABB)
inline Ray TransformRayIntoBoxLocalSpace(const Ray& worldRay, const OBB& obb)
{
	// Since the OBB orientation matrix is orthonormal (pure rotation, no scale or skew/distortion),
	// its inverse (transforming from world space to local space) is simply its transpose
	const glm::mat3 worldToLocalRotation = glm::transpose(obb.orientation);

	// Translate the ray's origin relative to the box's center 
	// so that the OBB center becomes the local origin (0, 0, 0)
	const glm::vec3 worldRayOriginFromBoxCenter = worldRay.origin - obb.center;

	// Apply the inverse rotation to both the position and the direction,
	// aligning them with the box's local coordinate system
	Ray localRay;
	localRay.origin    = worldToLocalRotation * worldRayOriginFromBoxCenter;
	localRay.direction = worldToLocalRotation * worldRay.direction;

	return localRay;
}

// Checks a single axis (X, Y, or Z) to see where the ray enters and exits the box walls ("slab")
inline bool NarrowHitRangeToAxisSlab(float rayOriginOnAxis, float rayDirectionOnAxis, float boxHalfExtentOnAxis,
	float& entryDistance, float& exitDistance)
{
	constexpr float NEARLY_PARALLEL_THRESHOLD = 1e-6f;

	// If the ray runs parallel to the walls, it only hits if the origin is already inside
	if (std::abs(rayDirectionOnAxis) < NEARLY_PARALLEL_THRESHOLD)
		return rayOriginOnAxis >= -boxHalfExtentOnAxis && rayOriginOnAxis <= boxHalfExtentOnAxis;

	// 't'        can be called 'distance'
	// 'direction can be called 'steps'
	//
	// Position     = Start     + t        * Direction
	// Position     = Start     + distance * steps
	// 
	// WallPosition =  RayOrigin    + distance   * RayDirection  
	// distance     = (WallPosition - RayOrigin) / RayDirection
	float distanceToNearFace = (-boxHalfExtentOnAxis  - rayOriginOnAxis) / rayDirectionOnAxis;
	float distanceToFarFace  = ( boxHalfExtentOnAxis  - rayOriginOnAxis) / rayDirectionOnAxis;

	// If the ray flies backwards on this axis, swap near and far faces
	if (distanceToNearFace > distanceToFarFace)
		std::swap(distanceToNearFace, distanceToFarFace);

	// Find the overlapping range (intersection) across all axes:
	// - Entry can only get later   (MAX)
	// - Exit  can only get earlier (MIN)
	entryDistance = std::max(entryDistance, distanceToNearFace);
	exitDistance  = std::min(exitDistance, distanceToFarFace);

	// If entry is further than exit, the ray missed the box entirely
	return entryDistance <= exitDistance;
}

// Raycasting using "slab method"
// https://en.wikipedia.org/wiki/Slab_method
// Finds where the ray enters/exits ONE axis slab (e.g. just X), then shrinks
// the shared [entryDistance, exitDistance] range to also fit that slab
// Returns false if the ray misses this slab entirely (can't hit the box then)
inline bool RayIntersectsOBB(const Ray& ray, const OBB& obb, float& outHitDistance)
{
	Ray localRay = TransformRayIntoBoxLocalSpace(ray, obb);

	float entryDistance = 0.0f;
	float exitDistance  = std::numeric_limits<float>::max();

	if (!NarrowHitRangeToAxisSlab(localRay.origin.x, localRay.direction.x, obb.halfExtents.x, entryDistance, exitDistance))
		return false;

	if (!NarrowHitRangeToAxisSlab(localRay.origin.y, localRay.direction.y, obb.halfExtents.y, entryDistance, exitDistance))
		return false;

	if (!NarrowHitRangeToAxisSlab(localRay.origin.z, localRay.direction.z, obb.halfExtents.z, entryDistance, exitDistance))
		return false;

	outHitDistance = entryDistance;
	return true;
}
 
// MOMENT OF INERTIA - how hard it is to make a shape SPIN, the same way mass
// is how hard it is to make it MOVE in a straight line. A long thin plank is
// much easier to spin end-over-end than side-over-side, this shows that
// per-axis difference
inline glm::vec3 ComputeBoxInertiaTensorLocal(const glm::vec3& halfExtents, float mass)
{
	glm::vec3 fullExtents = halfExtents * 2.0f;

	// Reference for return equation:
	// https://en.wikipedia.org/wiki/List_of_moments_of_inertia
	// See "rectangular cuboid"

	return glm::vec3(
		(mass / 12.0f) * (fullExtents.y * fullExtents.y + fullExtents.z * fullExtents.z), // resistance to spin around X
		(mass / 12.0f) * (fullExtents.x * fullExtents.x + fullExtents.z * fullExtents.z), // resistance to spin around Y
		(mass / 12.0f) * (fullExtents.x * fullExtents.x + fullExtents.y * fullExtents.y)  // resistance to spin around Z
	);
}

// Moment of inertia values only make sense measured along the box's OWN
// (possibly rotated) axes, but angular velocity is always in WORLD space
// 
// This "rotates" the per-axis resistance values into world space:
// first un-rotate a point back into the box's local space (orientation^T),
// apply the resistance there, then rotate the result back to world space
inline glm::mat3 ComputeInverseInertiaTensorWorld(const glm::vec3& inverseInertiaLocal, const glm::mat3& orientation)
{
	glm::mat3 inverseInertiaLocalMatrix = glm::mat3(
		inverseInertiaLocal.x, 0.0f, 0.0f,
		0.0f, inverseInertiaLocal.y, 0.0f,
		0.0f, 0.0f, inverseInertiaLocal.z
	);

	return orientation * inverseInertiaLocalMatrix * glm::transpose(orientation);
}

}