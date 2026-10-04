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

	UpdateKinematicVelocities(entityManager, frameDeltaTime);

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

void PhysicsSystem::WakeUp(PhysicsBodyComponent& physicsBody)
{
	physicsBody.isSleeping = false;
	physicsBody.timeSpentAlmostStill = 0.0f;
}

void PhysicsSystem::AddImpulse(PhysicsBodyComponent& physicsBody, const glm::vec3& impulse)
{
	// Protects from dividing by zero for a body with mass 0
	constexpr float SMALLEST_USABLE_MASS = 0.001f;

	AddVelocityChange(physicsBody, impulse / std::max(physicsBody.mass, SMALLEST_USABLE_MASS));
}

void PhysicsSystem::AddVelocityChange(PhysicsBodyComponent& physicsBody, const glm::vec3& velocityChange)
{
	if (physicsBody.isKinematic)
		return;

	physicsBody.velocity += velocityChange;
	WakeUp(physicsBody);
}

// Two ways to change the position of a kinematic body:
//
//  1. MovePosition(target) = REAL MOVEMENT
//     The body travels to the target and gets a velocity from the distance it covered,
//     so it can push other bodies that are in its way
//
//  2. Setting transform.position directly = TELEPORT
//     The body just appears at the new place. Its velocity stays 0, so it pushes nothing
//
// The caller chooses which one it wants, so physics never has to guess it from the distance
void PhysicsSystem::MovePosition(PhysicsBodyComponent& physicsBody, const glm::vec3& targetPosition)
{
	// Only kinematic bodies are moved by the caller
	// Normal bodies are moved by physics (use velocity or AddImpulse for them)
	if (!physicsBody.isKinematic)
		return;

	// If this is called more than once before the next update, the last call wins
	physicsBody.kinematicTarget = targetPosition;
	physicsBody.hasKinematicTarget = true;
}

void PhysicsSystem::RunSingleStep(EntityManager& entityManager, float deltaTime)
{
	RemoveSpinAroundFrozenAxes(entityManager);

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

// A kinematic body is moved by scripts, not by physics, so its 'velocity' would always stay 0
// Collisions are solved from velocities, so physics would see the body as standing still:
// it would never push or spin the bodies it hits, and it wouldn't wake up sleeping ones
//
// Here we carry out the move requested by MovePosition and set the velocity from it:
//     velocity = distance travelled / time
//
// If nobody requested a move this frame (the body is idle, or it was teleported), the velocity is 0
// Only straight movement is handled, spinning a kinematic body doesn't give it angular velocity
void PhysicsSystem::UpdateKinematicVelocities(EntityManager& entityManager, float frameDeltaTime)
{
	auto& physicsBodies = entityManager.GetAllComponents<PhysicsBodyComponent>();

	for (auto& physicsBody : physicsBodies)
	{
		if (!physicsBody.isKinematic)
		{
			// Forget any old request, so it can't be used if kinematic is turned on later
			physicsBody.hasKinematicTarget = false;
			continue;
		}

		glm::vec3 velocity = glm::vec3(0.0f);

		if (physicsBody.hasKinematicTarget && frameDeltaTime > 0.0f)
		{
			unsigned int entityID = entityManager.GetEntityIDForComponent(physicsBody);

			if (auto* transform = entityManager.GetComponent<TransformComponent>(entityID))
			{
				velocity = (physicsBody.kinematicTarget - transform->position) / frameDeltaTime;

				transform->position = physicsBody.kinematicTarget;
				transform->isDirty = true;
			}
		}

		// The request is used up, a new one is needed for the next move
		physicsBody.hasKinematicTarget = false;
		physicsBody.velocity = velocity;
	}
}

// Freeze Rotation: removes spin around the frozen axes (e.g. a player that must not fall over)
// Impacts can't ADD such spin (see GetInverseInertiaTensorWorld in CollisionSystem.cpp),
// so this only cleans values that were set from outside: the Inspector or a script
void PhysicsSystem::RemoveSpinAroundFrozenAxes(EntityManager& entityManager)
{
	auto& physicsBodies = entityManager.GetAllComponents<PhysicsBodyComponent>();

	for (auto& physicsBody : physicsBodies)
	{
		bool hasFrozenAxis = physicsBody.freezeRotationX || physicsBody.freezeRotationY || physicsBody.freezeRotationZ;

		if (!hasFrozenAxis || physicsBody.isKinematic || physicsBody.isSleeping)
			continue;

		unsigned int entityID = entityManager.GetEntityIDForComponent(physicsBody);

		auto* transform = entityManager.GetComponent<TransformComponent>(entityID);
		if (!transform)
			continue;

		// Angular velocity is in world space, but the frozen axes are the body's own,
		// so we move to the body's space, zero the frozen axes and move back
		glm::quat bodyRotation = transform->rotationQuaternion;
		glm::vec3 spinInBodySpace = glm::inverse(bodyRotation) * physicsBody.angularVelocity;

		if (physicsBody.freezeRotationX) spinInBodySpace.x = 0.0f;
		if (physicsBody.freezeRotationY) spinInBodySpace.y = 0.0f;
		if (physicsBody.freezeRotationZ) spinInBodySpace.z = 0.0f;

		physicsBody.angularVelocity = bodyRotation * spinInBodySpace;
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