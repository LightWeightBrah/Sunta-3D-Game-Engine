#pragma once

namespace Sunta
{

class EntityManager;

// Picks the animation of every entity that has a CharacterAnimationComponent
class CharacterAnimationSystem
{
public:
	// Call once per frame, AFTER physics (it reads the speed of the character) and BEFORE the animators are updated
	static void Update(EntityManager& entityManager);

	// Adds the Character Animation together with the Animator it needs
	// When the Model is already loaded, the animations called Idle, Walk and Run are chosen
	static void AddWithRequiredComponents(EntityManager& entityManager, unsigned int entityID);

	// Call when the game starts: explains in the log why a character would not animate (no Model, a missing animation etc.)
	static void ReportProblemsBeforePlay(EntityManager& entityManager);
};

}
