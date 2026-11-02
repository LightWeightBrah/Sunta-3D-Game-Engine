#pragma once

#include <string>
#include <glm/glm.hpp>

#include "ECS/ComponentLayout.h"
#include "Core/EngineAssets.h"

namespace Sunta
{

// What the player collects from defeated enemies (the souls of Dark Souls, the runes of Elden Ring: the game names it)
//
// How to use it: Add Component > Currency on the player, Add Component > Currency Reward on the enemies (how much each gives)
// 'displayName' is shown next to the amount in the HUD. With 'isDroppedOnDeath' (as in Dark Souls) dying leaves all of it
// where the player fell: walking back there gets it back, dying again on the way loses it for good
// 'dropMarker' is the name of an entity of the scene that shows the spot (e.g. one with a glowing Particle Emitter):
// it is moved there and its emitters run while something lies there. Within 'retrieveDistance' meters the player
// picks it up: with E and 'retrievePrompt' shown when 'isRetrievedWithKey', just by walking over it otherwise
// 'gainSound' plays when the amount grows (a kill, picking it up), 'retrieveSound' when the dropped amount is picked up
// A script can read and change it: this.currency.amount, this.currency:add(100)
struct CurrencyComponent
{
	// Limit of the slider in the Inspector
	static constexpr float MaxRetrieveDistance = 10.0f;

	std::string displayName = "Souls";
	int         amount      = 0;

	bool        isDroppedOnDeath   = true;
	std::string dropMarker;
	float       retrieveDistance   = 1.5f;
	bool        isRetrievedWithKey = true;
	std::string retrievePrompt     = "Retrieve souls";

	std::string gainSound;
	std::string retrieveSound;

	// ========== Filled while the game plays, not saved with the scene ==========

	int       droppedAmount   = 0;
	glm::vec3 droppedPosition = glm::vec3(0.0f);

	// The HUD counts up to the new amount and shows what was just gained above it for a moment
	float shownAmount        = 0.0f;
	int   lastGain           = 0;
	float lastGainSecondsLeft = 0.0f;

	bool HasDrop() const { return droppedAmount > 0; }

	static void RegisterToInspector()
	{
		InspectorComponentRegistry::RegisterComponent<CurrencyComponent>("Currency", EngineAssets::Icons::SoulsPlayer,
			{
				ADD_PROPERTY(CurrencyComponent, displayName, PropertyDataType::String),
				ADD_PROPERTY(CurrencyComponent, amount, PropertyDataType::Int),
				ADD_PROPERTY(CurrencyComponent, isDroppedOnDeath, PropertyDataType::Bool),
				ADD_PROPERTY(CurrencyComponent, dropMarker, PropertyDataType::String),
				ADD_PROPERTY_RANGED(CurrencyComponent, retrieveDistance, PropertyDataType::Float, 0.0f, MaxRetrieveDistance),
				ADD_PROPERTY(CurrencyComponent, isRetrievedWithKey, PropertyDataType::Bool),
				ADD_PROPERTY(CurrencyComponent, retrievePrompt, PropertyDataType::String),
				ADD_PROPERTY(CurrencyComponent, gainSound, PropertyDataType::String),
				ADD_PROPERTY(CurrencyComponent, retrieveSound, PropertyDataType::String)
			});
	}
};

// How much Currency a character (or anything with Health) gives to the player who defeats it
//
// How to use it: Add Component > Currency Reward on an enemy. It is given each time the enemy falls by the player's hand,
// so enemies brought back at a bonfire give it again (a defeated boss stays dead and gives it once)
struct CurrencyRewardComponent
{
	int amount = 50;

	static void RegisterToInspector()
	{
		InspectorComponentRegistry::RegisterComponent<CurrencyRewardComponent>("Currency Reward", EngineAssets::Icons::SoulsPlayer,
			{
				ADD_PROPERTY(CurrencyRewardComponent, amount, PropertyDataType::Int)
			});
	}
};

}
