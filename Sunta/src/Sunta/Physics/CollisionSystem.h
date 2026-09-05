#pragma once

#include <vector>

#include "CollisionShapes.h"
#include "SAT.h"

namespace Sunta
{

class EntityManager;

class CollisionSystem
{
public:
	static inline std::vector<OverlapPair> previousOverlaps;

	static void UpdateColliders(EntityManager& entityManager);
	static void DetectTriggerEvents(EntityManager& entityManager);
};

}