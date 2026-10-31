#include "Core/SuntaPreCompiled.h"
#include "LineOfSight.h"

#include "GameplayMath.h"
#include "SightBlockers.h"
#include "Physics/CollisionShapes.h"

namespace Sunta
{

bool IsLineOfSightBlocked(EntityManager& entityManager, const glm::vec3& from, const glm::vec3& to,
	unsigned int viewerEntityID, unsigned int targetEntityID, RaycastHit& outHit)
{
	const glm::vec3 path   = to - from;
	const float     length = glm::length(path);

	if (length < GameplayMath::SmallestUsableLength)
		return false;

	Ray ray;
	ray.origin    = from;
	ray.direction = path / length;

	return Physics::Raycast(entityManager, ray, length, SightBlockingLayers, { viewerEntityID, targetEntityID }, outHit);
}

}
