#include "Core/SuntaPreCompiled.h"
#include "PhysicsSystem.h"

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <algorithm>
#include <cmath>

#include "ECS/EntityManager.h"
#include "ECS/Component.h"
#include "ECS/Systems.h"
#include "CollisionSystem.h"

namespace Sunta
{

void PhysicsSystem::UpdatePhysics(EntityManager& entityManager, float frameDeltaTime)
{
	// Cap the frame time so sudden lags don't trigger a loop of 
	// too many expensive physics calculations that could freeze the game again
	// It's called "spiral of death" in Physics Engines
	constexpr float LONGEST_ACCEPTABLE_FRAME = 0.25f;
	frameDeltaTime = std::min(frameDeltaTime, LONGEST_ACCEPTABLE_FRAME);

	timeAccumulator += frameDeltaTime;

	// Run physics in steady, fixed-size steps 
	// This ensures movement is smooth, stable, and predictable, 
	// regardless of how fast or slow the computer is running (FPS)
	while (timeAccumulator >= FIXED_TIME_STEP)
	{
		RunSingleStep(entityManager, FIXED_TIME_STEP);
		timeAccumulator -= FIXED_TIME_STEP;
	}
}

void PhysicsSystem::RunSingleStep(EntityManager& entityManager, float deltaTime)
{
	ApplyLinearMotion(entityManager, deltaTime);
	ApplyAngularMotion(entityManager, deltaTime);
	ApplyDamping(entityManager, deltaTime);

	Systems::UpdateTransform(entityManager);
	CollisionSystem::Update(entityManager, deltaTime);

	SnapNegligibleVelocities(entityManager);
}

namespace
{

constexpr float GRAVITY_ACCELERATION = 9.81f;
constexpr float MAX_FALL_SPEED = 50.0f;

// Rotations in quaternions can't be added like position (we can't do rotation += speed * deltaTime)
// Quaternions are combined through formula:
// newRotation = normalize(currentRotation + (spin * currentRotation * 0.5f * deltaTime));
// spin is the angular velocity written as quaternion
// WE MUST NORMALIZE QUATERNION as floating point rounding slowly makes the quaternion rotation 
// not valid (valid has always length 1)
glm::quat ApplyAngularVelocity(const glm::quat& currentRotation, const glm::vec3& angularVelocity, float deltaTime)
{
	glm::quat spin(0.0f, angularVelocity.x, angularVelocity.y, angularVelocity.z);

	glm::quat rateOfChange = spin * currentRotation;
	glm::quat newRotation  = currentRotation + rateOfChange * (0.5f * deltaTime);

	return glm::normalize(newRotation);
}

float GetDampingFactor(float damping, float deltaTime)
{
	return 1.0f / (1.0f + damping * deltaTime);
}

// Floating point math never lands on a perfect, clean 0. A body that
// SHOULD be resting still ends up with a velocity like 0.00003, which
// then flips sign every step or two from tiny rounding differences,
// visible in the editor as values flickering endlessly, and enough to
// keep the sleep system from ever considering the body "still enough".
// Anything below this speed is treated as noise, not real motion, and
// clamped straight to zero
glm::vec3 SnapTinyVelocityToZero(const glm::vec3& velocity)
{
	constexpr float NEGLIGIBLE_SPEED = 0.0005f; // meters (or radians) / second

	return glm::vec3(
		std::abs(velocity.x) < NEGLIGIBLE_SPEED ? 0.0f : velocity.x,
		std::abs(velocity.y) < NEGLIGIBLE_SPEED ? 0.0f : velocity.y,
		std::abs(velocity.z) < NEGLIGIBLE_SPEED ? 0.0f : velocity.z
	);
}

}

void PhysicsSystem::ApplyLinearMotion(EntityManager& entityManager, float deltaTime)
{
	auto& physicsBodies = entityManager.GetAllComponents<PhysicsBodyComponent>();

	for (auto& physicsBody : physicsBodies)
	{
		if (physicsBody.isKinematic || physicsBody.isSleeping)
			continue;

		if (physicsBody.useGravity)
		{
			physicsBody.velocity.y -= GRAVITY_ACCELERATION * deltaTime;
			physicsBody.velocity.y = std::max(physicsBody.velocity.y, -MAX_FALL_SPEED);
		}

		unsigned int entityID = entityManager.GetEntityIDForComponent(physicsBody);

		auto* transform = entityManager.GetComponent<TransformComponent>(entityID);
		if (!transform)
			continue;

		transform->position += physicsBody.velocity * deltaTime;
		transform->isDirty = true;
	}
}

void PhysicsSystem::ApplyAngularMotion(EntityManager& entityManager, float deltaTime)
{
	auto& physicsBodies = entityManager.GetAllComponents<PhysicsBodyComponent>();

	for (auto& physicsBody : physicsBodies)
	{
		if (physicsBody.isKinematic || physicsBody.isSleeping)
			continue;

		if (physicsBody.angularVelocity == glm::vec3(0.0f))
			continue;

		unsigned int entityID = entityManager.GetEntityIDForComponent(physicsBody);

		auto* transform = entityManager.GetComponent<TransformComponent>(entityID);
		if (!transform)
			continue;

		transform->rotationQuaternion = ApplyAngularVelocity(transform->rotationQuaternion,
			physicsBody.angularVelocity, deltaTime);
		transform->SyncEulerFromQuaternion();
		transform->isDirty = true;
	}
}

void PhysicsSystem::ApplyDamping(EntityManager& entityManager, float deltaTime)
{
	auto& physicsBodies = entityManager.GetAllComponents<PhysicsBodyComponent>();

	for (auto& physicsBody : physicsBodies)
	{
		if(physicsBody.isKinematic || physicsBody.isSleeping)
			continue;

		physicsBody.velocity	    *= GetDampingFactor(physicsBody.linearDamping,  deltaTime);
		physicsBody.angularVelocity *= GetDampingFactor(physicsBody.angularDamping, deltaTime);
	}

}

void PhysicsSystem::SnapNegligibleVelocities(EntityManager& entityManager)
{
	auto& physicsBodies = entityManager.GetAllComponents<PhysicsBodyComponent>();

	for (auto& physicsBody : physicsBodies)
	{
		physicsBody.velocity        = SnapTinyVelocityToZero(physicsBody.velocity);
		physicsBody.angularVelocity = SnapTinyVelocityToZero(physicsBody.angularVelocity);
	}
}

}