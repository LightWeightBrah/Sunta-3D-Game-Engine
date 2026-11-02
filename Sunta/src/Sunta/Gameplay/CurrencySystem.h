#pragma once

namespace Sunta
{

class EntityManager;

// The player's Currency (see CurrencyComponent): rewards for defeated enemies, the amount left where the player fell,
// picking it up again
class CurrencySystem
{
public:
	// When the game starts: the amount set in the scene, nothing lies anywhere, the drop marker hidden
	static void PrepareForPlay(EntityManager& entityManager);

	// Once per frame while the game plays: picking up what was dropped, the HUD's counting
	static void Update(EntityManager& entityManager, float deltaTime);

	// A character with Health fell: its Currency Reward goes to the attacker's side (the player), and a falling player
	// leaves its Currency behind
	static void OnDeath(EntityManager& entityManager, unsigned int deadEntityID, unsigned int attackerEntityID);

	// Adds to the amount (a reward, a script), shown in the HUD as a gain
	static void Add(EntityManager& entityManager, unsigned int entityID, int amount);
};

}
