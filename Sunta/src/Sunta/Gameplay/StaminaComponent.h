#pragma once

#include "ECS/ComponentLayout.h"
#include "Core/EngineAssets.h"

namespace Sunta
{

// What rolling, attacking and running cost (the Dark Souls stamina bar)
//
// How to use it: Add Component > Stamina on a character with Character Actions
// An action can start while any stamina is left (it may take the bar down to zero), running stops at zero
// Stamina comes back on its own a moment after it was last used, but not during an action
struct StaminaComponent
{
	// Limits of the sliders in the Inspector
	static constexpr float MaxStaminaLimit = 1000.0f;
	static constexpr float MaxCost         = 200.0f;
	static constexpr float MaxDelaySeconds = 5.0f;

	float maxStamina = 100.0f;

	// How much comes back per second, and how long after the last use it starts coming back
	float regenerationPerSecond = 45.0f;
	float regenerationDelaySeconds = 0.6f;

	float rollCost        = 18.0f;
	float backstepCost    = 12.0f;
	float lightAttackCost = 14.0f;
	float heavyAttackCost = 28.0f;
	float runCostPerSecond = 12.0f;

	// Filled with 'maxStamina' when the game starts. Not saved with the scene
	float currentStamina = 100.0f;
	float secondsSinceUsed = 0.0f;

	static void RegisterToInspector()
	{
		InspectorComponentRegistry::RegisterComponent<StaminaComponent>("Stamina", EngineAssets::Icons::Physics,
			{
				ADD_PROPERTY_RANGED(StaminaComponent, maxStamina,               PropertyDataType::Float, 1.0f, MaxStaminaLimit),
				ADD_PROPERTY_RANGED(StaminaComponent, regenerationPerSecond,    PropertyDataType::Float, 0.0f, MaxStaminaLimit),
				ADD_PROPERTY_RANGED(StaminaComponent, regenerationDelaySeconds, PropertyDataType::Float, 0.0f, MaxDelaySeconds),
				ADD_PROPERTY_RANGED(StaminaComponent, rollCost,                 PropertyDataType::Float, 0.0f, MaxCost),
				ADD_PROPERTY_RANGED(StaminaComponent, backstepCost,             PropertyDataType::Float, 0.0f, MaxCost),
				ADD_PROPERTY_RANGED(StaminaComponent, lightAttackCost,          PropertyDataType::Float, 0.0f, MaxCost),
				ADD_PROPERTY_RANGED(StaminaComponent, heavyAttackCost,          PropertyDataType::Float, 0.0f, MaxCost),
				ADD_PROPERTY_RANGED(StaminaComponent, runCostPerSecond,         PropertyDataType::Float, 0.0f, MaxCost),
				ADD_PROPERTY_RANGED(StaminaComponent, currentStamina,           PropertyDataType::Float, 0.0f, MaxStaminaLimit)
			});
	}
};

}
