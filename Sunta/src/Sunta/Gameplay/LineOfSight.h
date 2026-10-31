#pragma once

#include <glm/glm.hpp>

#include "Physics/Raycast.h"

namespace Sunta
{

class EntityManager;

// True when something that blocks the sight (walls, floors: see SightBlockers.h) stands between the two points
// The one who looks and the one looked at never block the sight themselves. 'outHit' says what was in the way
bool IsLineOfSightBlocked(EntityManager& entityManager, const glm::vec3& from, const glm::vec3& to,
	unsigned int viewerEntityID, unsigned int targetEntityID, RaycastHit& outHit);

}
