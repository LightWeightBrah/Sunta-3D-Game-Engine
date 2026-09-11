#include "Core/SuntaPreCompiled.h"
#include "PhysicsSystem.h"

#include <glm/glm.hpp>
#include <algorithm>

#include "ECS/EntityManager.h"
#include "ECS/Component.h"

namespace Sunta
{

namespace
{

constexpr float GRAVITY_ACCELERATION = 9.81f;
constexpr float MAX_FALL_SPEED = 50.0f;

}

void PhysicsSystem::UpdatePhysics(EntityManager& entityManager, float deltaTime)
{
	auto& physicsBodies = entityManager.GetAllComponents<PhysicsBodyComponent>();

	for (auto& physicsBody : physicsBodies)
	{
		if(physicsBody.isKinematic)
			continue;

		if (physicsBody.useGravity)
		{
			physicsBody.velocity.y -= GRAVITY_ACCELERATION * deltaTime;
			physicsBody.velocity.y = std::max(physicsBody.velocity.y, -MAX_FALL_SPEED);
		}

		unsigned int entityID = entityManager.GetEntityIDForComponent(physicsBody);

		auto* transform = entityManager.GetComponent<TransformComponent>(entityID);
		if(!transform)
			continue;

		transform->position += physicsBody.velocity * deltaTime;
		transform->isDirty = true;
	}
}

}