#pragma once

#include <string>

#include "ECS/ComponentLayout.h"
#include "Core/EngineAssets.h"

namespace Sunta
{

// What hurt a character: a weapon (a Hitbox), a spell (a Projectile), or anything else (a shockwave, a script)
enum class DamageKind
{
	Weapon,
	Spell,
	Other
};

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

	// What flies out where a weapon touches the body while a script keeps it from being hurt ('isInvulnerable': a boss
	// changing its phase, protected by its summon): sparks of a blow that glances off, so the player sees the blow
	// landed but did nothing. Empty = the blow passes without a sign
	std::string deflectEffect;

	// The killing blow plays in slow motion for this long (real seconds, 0 = not at all) at 'deathSlowMotionScale' speed
	// (a boss falling)
	float deathSlowMotionSeconds = 0.0f;
	float deathSlowMotionScale   = 0.3f;

	// How much of a weapon's or a spell's damage gets through (1 = all of it, 0 = none): a ghost that blades barely
	// touch, a golem that shrugs off magic. A script can change them during the game (a boss that adapts to what
	// it was hit with: this.health.spellDamageTaken = 0.2)
	float weaponDamageTaken = 1.0f;
	float spellDamageTaken  = 1.0f;

	// After a spell (a projectile, a blast) hurt the character, no other spell can hurt it for this long: a volley of
	// many small projectiles costs one hit, not all of them at once (0 = every projectile hurts)
	float spellImmunitySeconds = 0.0f;

	// Since a spell last hurt the character. Not saved with the scene
	float secondsSinceSpellHit = 1000.0f;

	// Set once the health reaches 0. Not saved with the scene
	bool isDead = false;

	// Set by a script, not saved with the scene: while 'isInvulnerable' nothing hurts the character (a boss changing
	// its phase), and no hit takes its health below 'minimumHealth' (a boss that can't fall before its last phase)
	bool  isInvulnerable = false;
	float minimumHealth  = 0.0f;

	float GetDamageTaken(DamageKind kind) const
	{
		switch (kind)
		{
		case DamageKind::Weapon: return weaponDamageTaken;
		case DamageKind::Spell:  return spellDamageTaken;
		default:                 return 1.0f;
		}
	}

	static void RegisterToInspector()
	{
		InspectorComponentRegistry::RegisterComponent<HealthComponent>("Health", EngineAssets::Icons::Bonfire,
			{
				ADD_PROPERTY_RANGED(HealthComponent, maxHealth,     PropertyDataType::Float, 1.0f, MaxHealthLimit),
				ADD_PROPERTY_RANGED(HealthComponent, currentHealth, PropertyDataType::Float, 0.0f, MaxHealthLimit),
				ADD_PROPERTY(HealthComponent, hitEffect, PropertyDataType::String),
				ADD_PROPERTY(HealthComponent, deflectEffect, PropertyDataType::String),
				ADD_PROPERTY_RANGED(HealthComponent, deathSlowMotionSeconds, PropertyDataType::Float, 0.0f, 5.0f),
				ADD_PROPERTY_RANGED(HealthComponent, deathSlowMotionScale,   PropertyDataType::Float, 0.05f, 1.0f),
				ADD_PROPERTY_RANGED(HealthComponent, weaponDamageTaken,      PropertyDataType::Float, 0.0f, 3.0f),
				ADD_PROPERTY_RANGED(HealthComponent, spellDamageTaken,       PropertyDataType::Float, 0.0f, 3.0f),
				ADD_PROPERTY_RANGED(HealthComponent, spellImmunitySeconds,   PropertyDataType::Float, 0.0f, 5.0f)
			});
	}
};

}
