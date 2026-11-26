#pragma once

#include <string>
#include <vector>

#include "ECS/ComponentLayout.h"
#include "Core/EngineAssets.h"

namespace Sunta
{

// Calling allies into a fight (as Elden Ring's spirit ashes, Overlord's minions): G sets the entities named in
// 'allies' (comma separated, e.g. "Court Knight Ally, Court Knight Ally") down at the player's side; they fight the
// enemies until they fall or 'lifetimeSeconds' pass, then fade away. 'maxCharges' calls between two rests at a
// bonfire (a rest also sends the allies away)
//
// How to use it: Add Component > Summon on the player. Each ally is an enemy of the scene used as a template: an Enemy
// with 'isAlly' and 'startsDormant' (it waits hidden, out of the way), its body on the player's collision layer and
// its weapon's Hitbox on the player's hitbox layer. 'summonEffect' bursts where each one appears (a Particle Emitter)
struct SummonComponent
{
	// Limits of the sliders in the Inspector
	static constexpr float MaxSecondsLimit = 600.0f;
	static constexpr int   MaxChargesLimit = 10;

	std::string allies;
	int         maxCharges      = 1;
	float       lifetimeSeconds = 45.0f;
	std::string summonEffect;
	std::string calledText = "Your court answers";

	// ========== Filled while the game plays, not saved with the scene ==========

	int charges = -1; // -1 = not yet filled (the first frame fills it with maxCharges)
	struct Called { unsigned int entityID; float secondsLeft; };
	std::vector<Called> called;

	static void RegisterToInspector()
	{
		InspectorComponentRegistry::RegisterComponent<SummonComponent>("Summon", EngineAssets::Icons::Bonfire,
			{
				ADD_PROPERTY(SummonComponent, allies, PropertyDataType::String),
				ADD_PROPERTY(SummonComponent, maxCharges, PropertyDataType::Int),
				ADD_PROPERTY_RANGED(SummonComponent, lifetimeSeconds, PropertyDataType::Float, 1.0f, MaxSecondsLimit),
				ADD_PROPERTY(SummonComponent, summonEffect, PropertyDataType::String),
				ADD_PROPERTY(SummonComponent, calledText, PropertyDataType::String)
			});
	}
};

}
