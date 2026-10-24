#pragma once

#include <limits>
#include <glm/glm.hpp>

#include "CollisionLayers.h"
#include "CollisionShapes.h"

namespace Sunta
{

class EntityManager;

struct RaycastHit
{
	unsigned int entityID = 0;
	float        distance = 0.0f;                // from the ray origin to the hit point
	glm::vec3    point    = glm::vec3(0.0f);
};

namespace Physics
{

constexpr unsigned int NoIgnoredEntity = std::numeric_limits<unsigned int>::max();

// Shoots a ray and finds the CLOSEST box collider it hits, no further than maxDistance
// Only colliders whose layer is inside layerMask are tested (triggers too, choose the layers you want),
// and the ignored entity is skipped (the shooter's own collider, so a ray that starts inside it doesn't hit itself)
// Used for: a wall between the player and the lock-on target, the camera pushed away from a wall, the ground under the player
// A ray that starts inside a collider hits it at distance 0
// The ray direction doesn't have to be normalized, but must not be zero. Returns false when nothing is hit
bool Raycast(EntityManager& entityManager, const Ray& ray, float maxDistance, CollisionMask layerMask,
	unsigned int ignoredEntityID, RaycastHit& outHit);

}

}
