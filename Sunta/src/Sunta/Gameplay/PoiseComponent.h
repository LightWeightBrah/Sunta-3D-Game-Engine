#pragma once

#include "ECS/ComponentLayout.h"
#include "Core/EngineAssets.h"

namespace Sunta
{

// How much a character can take before a hit makes it stagger (poise, as in Dark Souls)
//
// How to use it: Add Component > Poise on a character with Health. Every weapon hit takes the weapon's
// 'poiseDamage' (Hitbox) away from it. While some poise is left the character keeps doing what it does,
// when it runs out the character staggers and its poise is full again
// 'recoverAfterSeconds' after the last hit the poise is full again too
// A character without Poise staggers after every hit
struct PoiseComponent
{
	// Limits of the sliders in the Inspector
	static constexpr float MaxPoiseLimit   = 500.0f;
	static constexpr float MaxRecoverDelay = 30.0f;

	float maxPoise = 30.0f;

	float recoverAfterSeconds = 4.0f;

	// ========== Filled while the game plays, not saved with the scene ==========

	float currentPoise = 30.0f;

	float secondsSinceHit = 0.0f;

	static void RegisterToInspector()
	{
		InspectorComponentRegistry::RegisterComponent<PoiseComponent>("Poise", EngineAssets::Icons::Bonfire,
			{
				ADD_PROPERTY_RANGED(PoiseComponent, maxPoise,            PropertyDataType::Float, 0.0f, MaxPoiseLimit),
				ADD_PROPERTY_RANGED(PoiseComponent, recoverAfterSeconds, PropertyDataType::Float, 0.0f, MaxRecoverDelay)
			});
	}
};

}
