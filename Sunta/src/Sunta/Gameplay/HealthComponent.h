#pragma once

#include "ECS/ComponentLayout.h"
#include "Core/EngineAssets.h"

namespace Sunta
{

// How much damage a character can take before it dies
//
// How to use it: Add Component > Health on the player or an enemy. Weapons (Hitbox) hurt it through its Box Collider
// A hit plays the hit animation of the Character Actions (the character staggers), the last one the death animation
// While a roll is invulnerable (Character Actions), hits do nothing
struct HealthComponent
{
	// Limit of the slider in the Inspector
	static constexpr float MaxHealthLimit = 5000.0f;

	float maxHealth = 100.0f;

	// Filled with 'maxHealth' when the game starts. Shown in the Inspector, so it can be changed while testing
	float currentHealth = 100.0f;

	// Set once the health reaches 0. Not saved with the scene
	bool isDead = false;

	static void RegisterToInspector()
	{
		InspectorComponentRegistry::RegisterComponent<HealthComponent>("Health", EngineAssets::Icons::Bonfire,
			{
				ADD_PROPERTY_RANGED(HealthComponent, maxHealth,     PropertyDataType::Float, 1.0f, MaxHealthLimit),
				ADD_PROPERTY_RANGED(HealthComponent, currentHealth, PropertyDataType::Float, 0.0f, MaxHealthLimit)
			});
	}
};

}
