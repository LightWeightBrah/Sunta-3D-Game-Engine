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

static bool ContainsPair(const std::vector<OverlapPair>& pairs, const OverlapPair& target)
{
	for (const auto& pair : pairs)
	{
		if (pair == target)
			return true;
	}

	return false;
}

static void PublishTriggerEvent(EntityManager& entityManager, const OverlapPair& pair, TriggerEventType eventType)
{
	auto* colliderA = entityManager.GetComponent<BoxColliderComponent>(pair.entityA);

	bool isEntityATrigger = colliderA && colliderA->isTrigger;

	unsigned int triggerEntityID = isEntityATrigger ? pair.entityA : pair.entityB;
	unsigned int otherEntityID   = isEntityATrigger ? pair.entityB : pair.entityA;

	if (eventType == TriggerEventType::Enter)
		EventBus::Publish(TriggerEnterEvent{ triggerEntityID, otherEntityID });
	else
		EventBus::Publish(TriggerExitEvent{ triggerEntityID, otherEntityID });


}

void CollisionSystem::DetectTriggerEvents(EntityManager& entityManager)
{
	auto& colliders = entityManager.GetAllComponents<BoxColliderComponent>();

	std::vector<OverlapPair> currentOverlaps;

	for (unsigned int i = 0; i < colliders.size(); i++)
	{
		for (unsigned int j = i + 1; j < colliders.size(); j++)
		{
			bool pairContainsTrigger = colliders[i].isTrigger || colliders[j].isTrigger;

			if(!pairContainsTrigger)
				continue;

			if (Overlaps(colliders[i].worldOBB, colliders[j].worldOBB))
			{
				unsigned int entityI = entityManager.GetEntityIDForComponent(colliders[i]);
				unsigned int entityJ = entityManager.GetEntityIDForComponent(colliders[j]);

				currentOverlaps.push_back({ entityI, entityJ });
			}
		}
	}

	// Overlapping NOW but NOT LAST FRAME => pair just started touching
	for (const auto& pair : currentOverlaps)
	{
		if (!ContainsPair(previousOverlaps, pair))
			PublishTriggerEvent(entityManager, pair, TriggerEventType::Enter);
	}

	// Overlapping LAST FRAME but NOT NOW => pair just stopped touching
	for (const auto& pair : previousOverlaps)
	{
		if (!ContainsPair(currentOverlaps, pair))
			PublishTriggerEvent(entityManager, pair, TriggerEventType::Exit);
	}

	previousOverlaps = std::move(currentOverlaps);
}

bool CollisionSystem::IsEntityOverlapping(unsigned int enityID)
{
	for (const auto& pair : previousOverlaps)
	{
		if (pair.entityA == enityID || pair.entityB == enityID)
			return true;
	}

	return false;
}

}