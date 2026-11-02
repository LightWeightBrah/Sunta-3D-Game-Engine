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

	// Shakes every follow camera: 'strength' 0..1 is added to the shake going on (0.2 a sword hit, 0.6 a giant's slam)
	static void Shake(EntityManager& entityManager, float strength);

	// A shake from something that happens at 'worldPosition' (a slam into the ground): full next to it, none beyond 'radius'
	static void ShakeFrom(EntityManager& entityManager, float strength, const glm::vec3& worldPosition, float radius);

	// Adds the Follow Camera together with the Camera component it needs
	// A Camera that the entity already has is left as it is
	static void AddWithRequiredComponents(EntityManager& entityManager, unsigned int entityID);

	// Call when the game starts: explains in the log why the camera would not follow anything
	static void ReportProblemsBeforePlay(EntityManager& entityManager);
};

}
