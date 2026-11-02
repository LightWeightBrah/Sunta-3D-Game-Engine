#pragma once

#include <string>

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

	// What flies out where a weapon hits the body (blood, ash, bone dust): the name of an entity with a Particle Emitter,
	// it bursts at the point of the hit. Empty = nothing
	std::string hitEffect;

	// The killing blow plays in slow motion for this long (real seconds, 0 = not at all) at 'deathSlowMotionScale' speed
	// (a boss falling)
	float deathSlowMotionSeconds = 0.0f;
	float deathSlowMotionScale   = 0.3f;

	// Set once the health reaches 0. Not saved with the scene
	bool isDead = false;

	static void RegisterToInspector()
	{
		InspectorComponentRegistry::RegisterComponent<HealthComponent>("Health", EngineAssets::Icons::Bonfire,
			{
				ADD_PROPERTY_RANGED(HealthComponent, maxHealth,     PropertyDataType::Float, 1.0f, MaxHealthLimit),
				ADD_PROPERTY_RANGED(HealthComponent, currentHealth, PropertyDataType::Float, 0.0f, MaxHealthLimit),
				ADD_PROPERTY(HealthComponent, hitEffect, PropertyDataType::String),
				ADD_PROPERTY_RANGED(HealthComponent, deathSlowMotionSeconds, PropertyDataType::Float, 0.0f, 5.0f),
				ADD_PROPERTY_RANGED(HealthComponent, deathSlowMotionScale,   PropertyDataType::Float, 0.05f, 1.0f)
			});
	}
};

}
