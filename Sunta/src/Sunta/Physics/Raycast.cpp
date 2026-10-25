#include "Core/SuntaPreCompiled.h"
#include "Raycast.h"

#include <algorithm>

#include "ECS/Component.h"
#include "ECS/EntityManager.h"

namespace Sunta
{

namespace Physics
{

namespace
{

constexpr float MinRayDirectionLength = 1e-6f;

// The one place that does the work. 'isIgnored' tells which entities the ray passes through
template<typename IsIgnored>
bool RaycastIgnoring(EntityManager& entityManager, const Ray& ray, float maxDistance, CollisionMask layerMask,
	IsIgnored isIgnored, RaycastHit& outHit)
{
	const float rayDirectionLength = glm::length(ray.direction);

	if (maxDistance <= 0.0f || rayDirectionLength < MinRayDirectionLength)
		return false;

	// Distances only make sense when the direction has length 1
	Ray normalizedRay;
	normalizedRay.origin    = ray.origin;
	normalizedRay.direction = ray.direction / rayDirectionLength;

	bool  isAnythingHit    = false;
	float closestDistance  = maxDistance;

	for (BoxColliderComponent& collider : entityManager.GetAllComponents<BoxColliderComponent>())
	{
		if (!MaskContainsLayer(layerMask, collider.layer))
			continue;

		const unsigned int entityID = entityManager.GetEntityIDForComponent(collider);

		if (isIgnored(entityID))
			continue;

		const auto* worldMatrixComponent = entityManager.GetComponent<WorldMatrixComponent>(entityID);
		if (!worldMatrixComponent)
			continue;

		// Built from the current transform (not the box saved by the last physics step), so the answer is never a frame old
		const OBB worldBox = MakeWorldOBB(worldMatrixComponent->matrix, collider.localOffset, collider.halfExtents);

		float hitDistance = 0.0f;
		if (!RayIntersectsOBB(normalizedRay, worldBox, hitDistance))
			continue;

		if (hitDistance > closestDistance)
			continue;

		isAnythingHit   = true;
		closestDistance = hitDistance;

		outHit.entityID = entityID;
		outHit.distance = hitDistance;
		outHit.point    = normalizedRay.origin + normalizedRay.direction * hitDistance;
	}

	return isAnythingHit;
}

}

bool Raycast(EntityManager& entityManager, const Ray& ray, float maxDistance, CollisionMask layerMask,
	unsigned int ignoredEntityID, RaycastHit& outHit)
{
	return RaycastIgnoring(entityManager, ray, maxDistance, layerMask,
		[ignoredEntityID](unsigned int entityID) { return entityID == ignoredEntityID; }, outHit);
}

bool Raycast(EntityManager& entityManager, const Ray& ray, float maxDistance, CollisionMask layerMask,
	std::initializer_list<unsigned int> ignoredEntityIDs, RaycastHit& outHit)
{
	return RaycastIgnoring(entityManager, ray, maxDistance, layerMask,
		[ignoredEntityIDs](unsigned int entityID)
		{
			return std::find(ignoredEntityIDs.begin(), ignoredEntityIDs.end(), entityID) != ignoredEntityIDs.end();
		}, outHit);
}

}

}
