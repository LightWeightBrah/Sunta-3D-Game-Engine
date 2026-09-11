#include "Core/SuntaPreCompiled.h"
#include "CollisionSystem.h"

#include "ECS/EntityManager.h"
#include "ECS/Component.h"
#include "Events/EventBus.h"
#include "Events/EventTypes.h"

namespace Sunta
{

void CollisionSystem::Update(EntityManager& entityManager)
{
	UpdateColliders(entityManager);
	ResolveSolidCollisions(entityManager);
	DetectTriggerEvents(entityManager);
}

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

namespace
{

// Resolve as solid only collisions without any trigger in pair, which layers see each other
bool ShouldResolveAsSolid(const BoxColliderComponent& a, const BoxColliderComponent& b)
{
	if (a.isTrigger || b.isTrigger)
		return false;

	return LayersCanInteract(a.layer, a.collidesWith, b.layer, b.collidesWith);
}

// If no physics body or isKinematic = unmovable object (wall etc.)
bool CanBePushed(const PhysicsBodyComponent* physicsBody)
{
	return physicsBody != nullptr && !physicsBody->isKinematic;
}

constexpr float EQUAL_PUSH_SHARE_BETWEEN_TWO_MOVABLE_BODIES = 0.5f;

void MoveEntityAndKeepColliderInSync(EntityManager& entityManager, unsigned int entityID,
	BoxColliderComponent& collider, const glm::vec3& movement)
{
	auto* transform = entityManager.GetComponent<TransformComponent>(entityID);
	if (!transform)
		return;

	transform->position += movement;
	transform->isDirty = true;

	collider.worldOBB.center += movement;
}
// Stops velocity from pushing object into a surface it's touching (e.g. floor)
// while keeping the part of velocity that slides along the surface
// Without this gravity would keep pushing the object into the floor every frame
// and PushEntitiesApart would keep pushing it back out, causing jittering
glm::vec3 StopVelocityGoingIntoSurface(const glm::vec3& velocity, const glm::vec3& surfaceNormal)
{
	// Dot product tells us how much velocity points toward the surface
	// Positive (+) = moving away from surface
	// Negative (-) = moving into the surface
	float velocityTowardsSurface = glm::dot(velocity, surfaceNormal);

	// postive dot product = moving away from surface (so don't do anything)
	if (velocityTowardsSurface >= 0.0f)
		return velocity;

	// Subtract only the "into surface" part of velocity , keep the rest untouched (slinding along surface)
	return velocity - surfaceNormal * velocityTowardsSurface;
}

void PushEntitiesApart(EntityManager& entityManager, BoxColliderComponent& colliderA, unsigned int entityA, BoxColliderComponent& colliderB, unsigned int entityB, const SeparationInfo& separationInfo)
{
	auto* physicsBodyA = entityManager.GetComponent<PhysicsBodyComponent>(entityA);
	auto* physicsBodyB = entityManager.GetComponent<PhysicsBodyComponent>(entityB);

	bool canPushA = CanBePushed(physicsBodyA);
	bool canPushB = CanBePushed(physicsBodyB);

	if (!canPushA && !canPushB)
		return;

	bool bothSidesCanMove = canPushA && canPushB;

	// "For every action (force) in nature there is an equal and opposite reaction" ~Sir Isaac Newton~
	// pushDirectionFromAToB points from A to B, so A is pushed backward (-) and B forward (+)
	if (canPushA)
	{
		float pushShare = bothSidesCanMove ? EQUAL_PUSH_SHARE_BETWEEN_TWO_MOVABLE_BODIES : 1.0f;
		glm::vec3 movementForA = -separationInfo.pushDirectionFromAToB * separationInfo.overlapDepth * pushShare;

		MoveEntityAndKeepColliderInSync(entityManager, entityA, colliderA, movementForA);
		physicsBodyA->velocity = StopVelocityGoingIntoSurface(physicsBodyA->velocity, -separationInfo.pushDirectionFromAToB);
	}

	if (canPushB)
	{
		float pushShare = bothSidesCanMove ? EQUAL_PUSH_SHARE_BETWEEN_TWO_MOVABLE_BODIES : 1.0f;
		glm::vec3 movementForB = separationInfo.pushDirectionFromAToB * separationInfo.overlapDepth * pushShare;

		MoveEntityAndKeepColliderInSync(entityManager, entityB, colliderB, movementForB);
		physicsBodyB->velocity = StopVelocityGoingIntoSurface(physicsBodyB->velocity, separationInfo.pushDirectionFromAToB);
	}

}

}

void CollisionSystem::ResolveSolidCollisions(EntityManager& entityManager)
{
	auto& colliders = entityManager.GetAllComponents<BoxColliderComponent>();

	for (unsigned int i = 0; i < colliders.size(); i++)
	{
		for (unsigned int j = i + 1; j < colliders.size(); j++)
		{
			if (!ShouldResolveAsSolid(colliders[i], colliders[j]))
				continue;

			SeparationInfo separationInfo = GetSeparationInfo(colliders[i].worldOBB, colliders[j].worldOBB);

			if (!separationInfo.areOverlapping)
				continue;

			unsigned int entityI = entityManager.GetEntityIDForComponent(colliders[i]);
			unsigned int entityJ = entityManager.GetEntityIDForComponent(colliders[j]);

			PushEntitiesApart(entityManager, colliders[i], entityI, colliders[j], entityJ, separationInfo);
		}
	}
}

namespace
{

bool ContainsPair(const std::vector<OverlapPair>& pairs, const OverlapPair& target)
{
	for (const auto& pair : pairs)
	{
		if (pair == target)
			return true;
	}

	return false;
}

void PublishTriggerEvent(EntityManager& entityManager, const OverlapPair& pair, TriggerEventType eventType)
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

			bool layersCanInteract = LayersCanInteract(colliders[i].layer, colliders[i].collidesWith,
											           colliders[j].layer, colliders[j].collidesWith);

			if(!layersCanInteract)
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