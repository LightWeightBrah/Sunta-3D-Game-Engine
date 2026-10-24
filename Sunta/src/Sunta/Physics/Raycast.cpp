#include "Core/SuntaPreCompiled.h"
#include "Raycast.h"

#include "ECS/Component.h"
#include "ECS/EntityManager.h"

namespace Sunta
{

namespace Physics
{

namespace
{

constexpr float MinRayDirectionLength = 1e-6f;

}

bool Raycast(EntityManager& entityManager, const Ray& ray, float maxDistance, CollisionMask layerMask,
	unsigned int ignoredEntityID, RaycastHit& outHit)
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

		if (entityID == ignoredEntityID)
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

}
