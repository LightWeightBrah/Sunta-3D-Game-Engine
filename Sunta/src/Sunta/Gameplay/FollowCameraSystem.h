#pragma once

#include <glm/glm.hpp>

namespace Sunta
{

class EntityManager;

// Moves every entity that has a FollowCameraComponent: it circles around the player and follows it
class FollowCameraSystem
{
public:
	// Call once per frame, AFTER the transforms are updated (the camera needs the final place of the player and of the walls)
	// 'lookDelta' is how far the mouse moved in this frame (pixels), zero when the game doesn't own the mouse
	static void Update(EntityManager& entityManager, float deltaTime, const glm::vec2& lookDelta);

	// Adds the Follow Camera together with the Camera component it needs
	// A Camera that the entity already has is left as it is
	static void AddWithRequiredComponents(EntityManager& entityManager, unsigned int entityID);

	// Call when the game starts: explains in the log why the camera would not follow anything
	static void ReportProblemsBeforePlay(EntityManager& entityManager);
};

}
