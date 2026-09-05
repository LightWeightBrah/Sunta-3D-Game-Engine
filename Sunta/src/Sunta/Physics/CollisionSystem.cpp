#include "Core/SuntaPreCompiled.h"
#include "CollisionSystem.h"

#include "ECS/EntityManager.h"
#include "ECS/Component.h"
#include "Events/EventBus.h"
#include "Events/EventTypes.h"

namespace Sunta
{

void CollisionSystem::UpdateColliders(EntityManager& entityManager)
{
	auto& colliders = entityManager.GetAllComponents<BoxColliderComponent>();

	for (unsigned int i = 0; i < colliders.size(); i++)
	{
		unsigned int entityID = entityManager.GetEntityIDForComponent(colliders[i]);

		auto* worldMatrixComponent = entityManager.GetComponent<WorldMatrixComponent>(entityID);
		if (!worldMatrixComponent)
			continue;

		colliders[i].worldOBB = MakeWorldOBB(worldMatrixComponent->matrix, colliders[i].localOffset, colliders[i].halfExtents);

	}
}

void CollisionSystem::DetectTriggerEvents(EntityManager& entityManager)
{

}

}