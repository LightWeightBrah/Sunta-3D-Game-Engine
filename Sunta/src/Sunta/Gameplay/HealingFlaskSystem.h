#pragma once

namespace Sunta
{

class EntityManager;

// Drinking from Healing Flasks (see HealingFlaskComponent): R for the player, Drink for a script, the heal in the
// middle of the drink
class HealingFlaskSystem
{
public:
	// When the game starts: every flask full
	static void PrepareForPlay(EntityManager& entityManager);

	// Once per frame while the game plays, after Character Actions (the drink's progress is known)
	static void Update(EntityManager& entityManager);

	// Starts a drink. False when the flask is empty or the character can't drink right now (busy, dead)
	static bool Drink(EntityManager& entityManager, unsigned int entityID);

	// Fills the flask of the character (a rest at a bonfire, coming back after death)
	static void Refill(EntityManager& entityManager, unsigned int entityID);
};

}
