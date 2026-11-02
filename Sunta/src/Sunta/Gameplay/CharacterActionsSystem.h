#pragma once

#include <string>

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
	// Drinking from a flask or using any item: the animation plays once on the upper body, the character walks slowly.
	// False when the character is busy (attacking, rolling, staggering) or has no such animation
	static bool UseItem(EntityManager& entityManager, unsigned int entityID, const std::string& animationName);

	static void PlayDeath(EntityManager& entityManager, unsigned int entityID);

	// Started by a hit: taken on the shield (the character stays behind it), or the attacker's own attack was parried
	// (a long stagger, the next hit it takes is a riposte)
	static void PlayBlockImpact(EntityManager& entityManager, unsigned int entityID);
	static void PlayParried(EntityManager& entityManager, unsigned int entityID);

	// Plays any animation of the character once (a roar, a taunt), interrupting what it does. It can't be cut short and
	// hits don't stagger it. False when the character has no such animation (or is dead)
	static bool PlaySpecial(EntityManager& entityManager, unsigned int entityID, const std::string& animationName);

	// Started by a bonfire: the character kneels (moving, rolling and attacking do nothing) until StandUp,
	// which plays the animation of getting up (or just ends the rest when there is none)
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
