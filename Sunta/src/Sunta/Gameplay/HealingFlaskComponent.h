#pragma once

#include <string>

#include "ECS/ComponentLayout.h"
#include "Core/EngineAssets.h"

namespace Sunta
{

// A flask that heals with a few drinks, filled again at every rest (the Estus Flask of Dark Souls, the Flask of Crimson
// Tears of Elden Ring: the game gives it its own name)
//
// How to use it: Add Component > Healing Flask on the player (or an enemy that drinks). 'displayName' is what the HUD shows,
// 'maxCharges' how many drinks it holds, each heals 'healAmount' health (at most up to the maximum)
// The player drinks with R: 'drinkAnimation' plays once on the upper body, the character walks slowly meanwhile and the
// health comes at 'healAtProgress' of the animation. A hit before that stops the drink and the charge is not used.
// Without an animation the drink heals at once
// 'hiddenWhileDrinking' is the name of a child entity whose Model disappears for the drink (the shield in the drinking
// hand), 'shownWhileDrinking' one that appears only then (a model of the flask held in that hand); empty = none
// 'healEffect' is the name of a child entity with a Particle Emitter that bursts with the heal (a warm glow), 'drinkSound'
// and 'healSound' are sound files or folders (see Audio.h). Resting at a bonfire and coming back after death fill the flask
// A script can use it: this.healingFlask:drink(), this.healingFlask.charges, this.healingFlask.maxCharges
struct HealingFlaskComponent
{
	// Limits of the sliders in the Inspector
	static constexpr int   MaxChargesLimit = 99;
	static constexpr float MaxHealAmount   = 5000.0f;

	std::string displayName    = "Healing Flask";
	int         maxCharges     = 5;
	float       healAmount     = 60.0f;

	std::string drinkAnimation;
	float       healAtProgress = 0.55f;

	std::string hiddenWhileDrinking;
	std::string shownWhileDrinking;

	std::string healEffect;
	std::string drinkSound;
	std::string healSound;

	// ========== Filled while the game plays, not saved with the scene ==========

	int  charges    = 5;
	bool isDrinking = false;
	bool hasHealed  = false;

	static void RegisterToInspector()
	{
		InspectorComponentRegistry::RegisterComponent<HealingFlaskComponent>("Healing Flask", EngineAssets::Icons::Bonfire,
			{
				ADD_PROPERTY(HealingFlaskComponent, displayName, PropertyDataType::String),
				ADD_PROPERTY(HealingFlaskComponent, maxCharges, PropertyDataType::Int),
				ADD_PROPERTY_RANGED(HealingFlaskComponent, healAmount, PropertyDataType::Float, 0.0f, MaxHealAmount),
				ADD_PROPERTY(HealingFlaskComponent, drinkAnimation, PropertyDataType::String),
				ADD_PROPERTY_RANGED(HealingFlaskComponent, healAtProgress, PropertyDataType::Float, 0.0f, 1.0f),
				ADD_PROPERTY(HealingFlaskComponent, hiddenWhileDrinking, PropertyDataType::String),
				ADD_PROPERTY(HealingFlaskComponent, shownWhileDrinking, PropertyDataType::String),
				ADD_PROPERTY(HealingFlaskComponent, healEffect, PropertyDataType::String),
				ADD_PROPERTY(HealingFlaskComponent, drinkSound, PropertyDataType::String),
				ADD_PROPERTY(HealingFlaskComponent, healSound, PropertyDataType::String)
			});
	}
};

}
