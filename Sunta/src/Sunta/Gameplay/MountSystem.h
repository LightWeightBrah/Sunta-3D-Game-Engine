#pragma once

#include <glm/glm.hpp>

namespace Sunta
{

class EntityManager;

// Riding (see MountComponent): mounting and dismounting with E, steering the ridden mount, the rider in the saddle
class MountSystem
{
public:
	// Call when the game starts: nobody rides
	static void PrepareForPlay(EntityManager& entityManager);

	// Before the character controllers: E near a mount, and the keys steer the one ridden
	static void Update(EntityManager& entityManager, float deltaTime, const glm::vec3& cameraForward);

	// After the physics, before the animations: the rider sits in the saddle where the mount is now
	static void PlaceRiders(EntityManager& entityManager);

	// The player rides something now (scripts, the HUD)
	static bool IsRiding(EntityManager& entityManager, unsigned int riderID);

	// Gets off at once (a script: a cutscene, a fall, a blow that throws the rider)
	static void Dismount(EntityManager& entityManager, unsigned int mountID);
};

}
