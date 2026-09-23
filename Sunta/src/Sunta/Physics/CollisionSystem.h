#pragma once

#include <vector>

#include "CollisionShapes.h"
#include "SAT.h"

namespace Sunta
{

class EntityManager;

enum class TriggerEventType
{
	Enter,
	Exit
};

class CollisionSystem
{
public:
	static void Update(EntityManager& entityManager, float deltaTime);

	static void UpdateColliders(EntityManager& entityManager);
	static void ResolveSolidCollisions(EntityManager& entityManager, float deltaTime);
	static void DetectTriggerEvents(EntityManager& entityManager);

	static void Reset() { previousOverlaps.clear(); }

	static bool IsEntityOverlapping(unsigned int enityID);

	static void ForgetEntity(unsigned int entityID);

private:
	static inline std::vector<OverlapPair> previousOverlaps;
	
};

}