#pragma once

#include <string>

namespace Sunta
{

class EntityManager;
struct HUDArea;

// The three attributes of the Level Up component, each with its own effect in the engine
enum class LevelUpAttribute
{
	Health,
	Stamina,
	Damage
};

// Raising the level with the Currency at a bonfire (see LevelUpComponent)
class LevelUpSystem
{
public:
	// Call when the game starts, before the health and stamina are filled: the points raise the maximum health and stamina
	static void PrepareForPlay(EntityManager& entityManager);

	// Call once per frame while the game plays, after the bonfires: the menu's keys while the player rests
	static void Update(EntityManager& entityManager, float deltaTime);

	// Pays the price of the next level and raises the attribute by one point. False when the Currency is too low
	static bool Raise(EntityManager& entityManager, unsigned int entityID, LevelUpAttribute attribute);

	// "health", "stamina" or "damage" (scripts). False for any other word
	static bool TryParseAttribute(const std::string& word, LevelUpAttribute& outAttribute);

	// How much the attribute points raise the weapon damage of the character (1 without Level Up)
	static float GetDamageMultiplier(EntityManager& entityManager, unsigned int entityID);

	// Draws the menu while the player rests at a bonfire (call between UIRenderer::Begin and End)
	static void DrawMenu(EntityManager& entityManager, const HUDArea& area);
};

}
