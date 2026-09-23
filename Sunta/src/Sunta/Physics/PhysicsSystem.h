#pragma once

namespace Sunta
{

class EntityManager;

class PhysicsSystem
{
public:
	static constexpr float FIXED_TIME_STEP = 1.0f / 60.0f;

	static void UpdatePhysics(EntityManager& entityManager, float deltaTime);

private:
	static inline float timeAccumulator = 0.0f;

	static void RunSingleStep(EntityManager& entityManager, float frameDeltaTime);

	static void ApplyLinearMotion(EntityManager& entityManager, float deltaTime);
	static void ApplyAngularMotion(EntityManager& entityManager, float deltaTime);
	static void ApplyDamping(EntityManager& entityManager, float deltaTime);
	static void SnapNegligibleVelocities(EntityManager& entityManager);
};

}