#pragma once

namespace Sunta
{

class EntityManager;

class PhysicsSystem
{
public:
	static void UpdatePhysics(EntityManager& entityManager, float deltaTime);

private:
	static void ApplyLinearMotion(EntityManager& entityManager, float deltaTime);
	static void ApplyAngularMotion(EntityManager& entityManager, float deltaTime);
};

}