#pragma once

#include <string>

namespace Sunta
{

class EntityManager;

// Items lying in the world, picked up with E, books read aloud, loot dropped by the dead (see PickupComponent)
class PickupSystem
{
public:
	static void PrepareForPlay(EntityManager& entityManager);

	// Once per frame while the game plays
	static void Update(EntityManager& entityManager, float deltaTime);

	// The player's inventory (scripts): adds or takes one, counts
	static void AddItem(EntityManager& entityManager, unsigned int ownerID, const std::string& itemId);
	static bool RemoveItem(EntityManager& entityManager, unsigned int ownerID, const std::string& itemId);
	static int  CountItem(EntityManager& entityManager, unsigned int ownerID, const std::string& itemId);
};

}
