#pragma once

#include <glm/glm.hpp>

namespace Sunta
{

class EntityManager;

// Moves every entity that has a CharacterControllerComponent
class CharacterControllerSystem
{
public:
	// Call once per frame, BEFORE physics (the speed set here is then used by the physics step)
	// 'cameraForward' is where the camera looks: the player walks "forward" relative to it
	static void Update(EntityManager& entityManager, float deltaTime, const glm::vec3& cameraForward);

	// The entity the player plays: the first Character Controller with 'isPlayerControlled' turned on. False when there is none
	static bool TryFindPlayer(EntityManager& entityManager, unsigned int& outEntityID);

	// Adds the Character Controller together with the parts it needs and fills them with sensible values:
	// a Physics Body (upright, no friction) and a Box Collider (the size of a person)
	// Parts the entity already has are left as they are
	static void AddWithRequiredComponents(EntityManager& entityManager, unsigned int entityID);

	// Call when the game starts: explains in the log why a character would not move (missing Physics Body etc.)
	static void ReportProblemsBeforePlay(EntityManager& entityManager);
};

}
