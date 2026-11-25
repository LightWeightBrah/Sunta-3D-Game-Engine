#pragma once

namespace Sunta
{

class EntityManager;

// Lets the player rest at the nearest bonfire (see BonfireComponent)
class BonfireSystem
{
public:
	// Call when the game starts: no bonfire is lit yet
	static void PrepareForPlay(EntityManager& entityManager);

	// Call once per frame while the game plays: shows "Rest at the bonfire [E]" near a bonfire and rests on E, E again stands up
	static void Update(EntityManager& entityManager);

	// True while the player rests at a bonfire: Escape then means standing up (the menus of the bonfire are left),
	// not the pause menu
	static bool IsPlayerResting(EntityManager& entityManager);
};

}
