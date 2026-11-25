#pragma once

#include <string>

#include <glm/glm.hpp>

namespace Sunta
{

class EntityManager;

// Entities made and removed while the game plays (Lua world:spawn / world:destroy)
//
// A spawned entity is a copy of an entity of the scene used as a template (with its children): an enemy, a pickup,
// a summoned monster kept somewhere out of sight in the scene. The copy starts where it is asked to, its scripts from
// the next frame. Removing is done at the end of the frame, so no system loses the entity it is working on
class WorldSpawner
{
public:
	// The copy of the entity named 'templateName' at 'position', turned 'yawDegrees' around the vertical axis.
	// False when there is no such entity
	static bool Spawn(EntityManager& entityManager, const std::string& templateName, const glm::vec3& position,
		float yawDegrees, unsigned int& outEntityID);

	// The entity and everything under it are removed at the end of this frame
	static void RequestDestroy(unsigned int entityID);

	// Called once per frame by the scene, after every system
	static void DestroyRequested(EntityManager& entityManager);
};

}
