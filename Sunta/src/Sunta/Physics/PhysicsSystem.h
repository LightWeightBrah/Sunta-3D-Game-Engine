#pragma once

#include <glm/vec3.hpp>

namespace Sunta
{

class EntityManager;
struct PhysicsBodyComponent;

class PhysicsSystem
{
public:
	static constexpr float FIXED_TIME_STEP = 1.0f / 60.0f;

	static void UpdatePhysics(EntityManager& entityManager, float deltaTime);

	// Physics skips sleeping bodies, so anything that changes a body from outside
	// (script, collision) must wake it up first, or the change would be ignored
	static void WakeUp(PhysicsBodyComponent& physicsBody);

	// Instant change of velocity (heavier bodies change less)
	// Kinematic bodies ignore impulses, nothing pushes them
	static void AddImpulse(PhysicsBodyComponent& physicsBody, const glm::vec3& impulse);

	// Instant change of velocity, the same for every mass (use it when you know the speed you want, not the force)
	// Kinematic bodies ignore it
	static void AddVelocityChange(PhysicsBodyComponent& physicsBody, const glm::vec3& velocityChange);

	// Moves a kinematic body as real movement (it pushes other bodies on the way), unlike setting transform.position,
	// which is a teleport and pushes nothing. Normal bodies ignore it
	static void MovePosition(PhysicsBodyComponent& physicsBody, const glm::vec3& targetPosition);

private:
	static inline float timeAccumulator = 0.0f;

	static void RunSingleStep(EntityManager& entityManager, float frameDeltaTime);

	static void UpdateKinematicVelocities(EntityManager& entityManager, float frameDeltaTime);
	static void RemoveSpinAroundFrozenAxes(EntityManager& entityManager);
	static void ApplyLinearMotion(EntityManager& entityManager, float deltaTime);
	static void ApplyAngularMotion(EntityManager& entityManager, float deltaTime);
	static void ApplyDamping(EntityManager& entityManager, float deltaTime);
	static void SnapNegligibleVelocities(EntityManager& entityManager);
};

}