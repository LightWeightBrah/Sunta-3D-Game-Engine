#pragma once

#include <string>

#include "ECS/ComponentLayout.h"
#include "Core/EngineAssets.h"

namespace Sunta
{

// The health of an enemy on the screen
//
// How to use it: Add Component > Enemy Health Bar on an enemy with Health (the Rusted Knight has it)
// A normal enemy shows a small bar over its head for a while after it was hit, and while the player is locked on to it
// A boss ('isBoss' on) shows a long bar with its name at the bottom of the screen, from the moment it notices the player
struct EnemyHealthBarComponent
{
	// Limit of the slider in the Inspector
	static constexpr float MaxSeconds = 30.0f;

	bool isBoss = false;

	// The name over the boss bar. Empty: the name of the entity
	std::string displayName;

	// How long the small bar stays after a hit
	float showSecondsAfterHit = 5.0f;

	// How high over the enemy's feet the small bar floats (meters)
	float heightAboveFeet = 2.15f;

	// ========== Filled while the game plays, not saved with the scene ==========

	float trailingHealth  = 0.0f;
	float healthLastFrame = 0.0f;

	// Starts huge: a bar that was never hit is not shown
	float secondsSinceHit = MaxSeconds;

	// Set once the enemy noticed the player or was hit: from then on a boss bar stays on the screen
	bool isEngaged = false;

	static void RegisterToInspector()
	{
		InspectorComponentRegistry::RegisterComponent<EnemyHealthBarComponent>("Enemy Health Bar", EngineAssets::Icons::Bonfire,
			{
				ADD_PROPERTY(EnemyHealthBarComponent, isBoss,      PropertyDataType::Bool),
				ADD_PROPERTY(EnemyHealthBarComponent, displayName, PropertyDataType::String),
				ADD_PROPERTY_RANGED(EnemyHealthBarComponent, showSecondsAfterHit, PropertyDataType::Float, 0.0f, MaxSeconds),
				ADD_PROPERTY_RANGED(EnemyHealthBarComponent, heightAboveFeet,     PropertyDataType::Float, 0.0f, 10.0f)
			});
	}
};

}
