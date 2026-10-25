#pragma once

#include <glm/glm.hpp>

namespace Sunta
{

class EntityManager;
struct LockOnComponent;

// Decides which enemy (an entity with a Lock On Target component) every Lock On component holds
class LockOnSystem
{
public:
	// Call once per frame, BEFORE the character controller (it turns the character towards the enemy that is locked on to)
	// 'cameraForward' is where the camera looks: the enemy "in front of the camera" is the one Q locks on to
	// 'lookDelta' is how far the mouse moved in this frame (pixels), zero when the game doesn't own the mouse
	static void Update(EntityManager& entityManager, float deltaTime, const glm::vec3& cameraForward, const glm::vec2& lookDelta);

	// The point of the locked enemy that the camera and the character aim at. False when nothing is locked on to
	static bool TryGetTargetAimPoint(EntityManager& entityManager, const LockOnComponent& lockOn, glm::vec3& outAimPoint);

	// For scripts. False (and a warning in the log) when the entity can't be locked on to, because it has no Lock On Target
	static bool TrySetTarget(EntityManager& entityManager, LockOnComponent& lockOn, unsigned int targetEntityID);

	static void ClearTarget(LockOnComponent& lockOn);
};

}
