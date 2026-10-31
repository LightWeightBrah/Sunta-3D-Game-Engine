#pragma once

namespace Sunta
{

class EntityManager;

// Starts, plays and ends the rolls and attacks of every entity with a CharacterActionsComponent
class CharacterActionsSystem
{
public:
	// Call once per frame AFTER the Character Controller (it reads the held direction) and BEFORE physics (it sets the speed)
	// 'canUseMouse' is false while the mouse belongs to the editor, so clicking a window doesn't swing the sword
	static void Update(EntityManager& entityManager, bool canUseMouse);

	// Started by the Health: the character staggers (interrupting what it does), or dies and never acts again
	static void PlayHitReaction(EntityManager& entityManager, unsigned int entityID);
	static void PlayDeath(EntityManager& entityManager, unsigned int entityID);

	// Started by a bonfire: the character kneels (moving, rolling and attacking do nothing) until StandUp
	static void PlayRest(EntityManager& entityManager, unsigned int entityID);
	static void StandUp(EntityManager& entityManager, unsigned int entityID);

	// Back to no action at all (a character brought back to life): walking and its animations take over again
	static void Reset(EntityManager& entityManager, unsigned int entityID);

	// Adds Character Actions and picks the animations by their names when the Model is already loaded
	static void AddWithRequiredComponents(EntityManager& entityManager, unsigned int entityID);

	// Call when the game starts: explains in the log why a character would not roll or attack
	static void ReportProblemsBeforePlay(EntityManager& entityManager);
};

}
