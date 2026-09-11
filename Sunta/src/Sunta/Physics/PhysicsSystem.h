#pragma once

namespace Sunta
{

class EntityManager;

class PhysicsSystem
{
public:
	static void UpdatePhysics(EntityManager& entityManager, float deltaTime);
};

}