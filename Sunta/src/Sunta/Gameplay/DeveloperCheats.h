#pragma once

namespace Sunta
{

class EntityManager;

// Helps whoever makes the game try everything (only in the editor and in development builds, never in a release):
//  F1 - the player can't die: blows still land, stagger and hurt, but health never goes under a tenth of the maximum,
//       and stamina stays full (long fights, every phase and spell can be tried without starting over)
class DeveloperCheats
{
public:
	// Set by the editor and by a development build when they start
	static inline bool isAllowed = false;

	static inline bool isPlayerImmortal = false;

	// Call once per frame while the game plays
	static void Update(EntityManager& entityManager);

	// The lowest health the player may have now (0: no cheat)
	static float GetPlayerHealthFloor(float maxHealth);
};

}
