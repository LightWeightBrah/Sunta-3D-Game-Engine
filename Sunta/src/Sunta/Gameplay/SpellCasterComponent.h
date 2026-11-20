#pragma once

#include <string>

#include "ECS/ComponentLayout.h"
#include "Core/EngineAssets.h"

namespace Sunta
{

// A spell a character can cast: an animation, and at its moment the Projectile it throws (see ProjectileComponent)
//
// How to use it: Add Component > Spell Caster on a child entity of the character, placed where the spell leaves from
// (on a bone: the hand, the tip of a staff). The caster is the closest parent with Character Actions. A character with
// several spells has several such children (a fireball, a pillar of fire). A Melee Enemy casts a spell that is ready
// when the player is between 'minDistance' and 'maxDistance' and in sight, instead of walking up to it
//
// The cast is an action of the character: a hit staggers it and the spell is lost. Until 'releaseAt' the caster turns
// after its target (Character Actions 'attackTurnDegreesPerSecond'), then the projectiles leave at the target:
// 'projectileCount' of them fanned out by 'spreadDegrees' (a volley that is hard to side step), or with
// 'atTargetsFeet' they appear under the target (the first right under it, the others around it)
//
// The player casts a spell with its 'castKey' (a letter or a digit, e.g. R), at the locked-on enemy or straight ahead.
// With 'isCastOnRequestOnly' an enemy never chooses the spell itself: only a script casts it
// (spell.spellCaster:cast(), e.g. a boss's great spell at the start of its phase)
struct SpellCasterComponent
{
	// Limits of the sliders in the Inspector
	static constexpr float MaxDistance = 60.0f;
	static constexpr float MaxSeconds  = 30.0f;
	static constexpr float MaxSpread   = 90.0f;
	static constexpr float MaxCharge   = 1000.0f;

	// An animation of the caster's Model, played once on the whole body
	std::string castAnimation;

	// The name of the entity with the Projectile component this spell throws
	std::string projectile;

	int   projectileCount = 1;
	float spreadDegrees   = 0.0f;

	// The moment the projectiles leave, as a part of the animation (0 = its start, 1 = its end)
	float releaseAt = 0.5f;

	bool atTargetsFeet = false;

	// Above 0: the projectiles don't leave the caster's hand, they come straight down on the target from this high
	// above it (meters): a meteor, a bolt from the sky, something enormous lowered onto the enemy
	float dropHeight = 0.0f;

	// Cast only at a target this far away (meters), once every 'cooldownSeconds'
	float minDistance     = 3.0f;
	float maxDistance     = 14.0f;
	float cooldownSeconds = 6.0f;

	// The wind-up: specks of the emitter named 'chargeEffect' gather where the spell leaves from (per second)
	std::string chargeEffect;
	float       chargeParticlesPerSecond = 80.0f;

	// Played when the projectiles leave
	std::string castSound;

	// The key of the player's spell (its first character counts: a letter or a digit). Empty: no key
	std::string castKey;

	// What the HUD calls the player's spell next to its key (e.g. "Fireball"). Empty: the name of the entity
	std::string displayName;

	// Never chosen by the enemy's own thinking, only cast by a script
	bool isCastOnRequestOnly = false;

	// Hits don't stop its cast (hyper armor for the cast): the caster still loses health, but the spell goes on.
	// For the player's great techniques, which shouldn't be lost to any blow that lands during their wind-up
	bool isUninterruptible = false;

	// ========== Filled while the game plays, not saved with the scene ==========

	// Seconds until it can be cast again
	float cooldownLeft = 0.0f;

	// Its cast is going on, and whether the projectiles already left
	bool isCasting   = false;
	bool hasReleased = false;

	// Specks of the charge owed to the next frame (less than one a frame at a low rate)
	float unbornChargeParticles = 0.0f;

	// How many times its projectiles left since the game started (a script notices a finished cast by its change)
	int timesReleased = 0;

	bool IsReady() const { return cooldownLeft <= 0.0f && !isCasting; }

	static void RegisterToInspector()
	{
		InspectorComponentRegistry::RegisterComponent<SpellCasterComponent>("Spell Caster", EngineAssets::Icons::Particle,
			{
				ADD_PROPERTY(SpellCasterComponent, castAnimation,   PropertyDataType::String),
				ADD_PROPERTY(SpellCasterComponent, projectile,      PropertyDataType::String),
				ADD_PROPERTY(SpellCasterComponent, projectileCount, PropertyDataType::Int),
				ADD_PROPERTY_RANGED(SpellCasterComponent, spreadDegrees,            PropertyDataType::Float, 0.0f, MaxSpread),
				ADD_PROPERTY_RANGED(SpellCasterComponent, releaseAt,                PropertyDataType::Float, 0.0f, 1.0f),
				ADD_PROPERTY(SpellCasterComponent, atTargetsFeet,   PropertyDataType::Bool),
				ADD_PROPERTY_RANGED(SpellCasterComponent, dropHeight,               PropertyDataType::Float, 0.0f, 100.0f),
				ADD_PROPERTY_RANGED(SpellCasterComponent, minDistance,              PropertyDataType::Float, 0.0f, MaxDistance),
				ADD_PROPERTY_RANGED(SpellCasterComponent, maxDistance,              PropertyDataType::Float, 0.0f, MaxDistance),
				ADD_PROPERTY_RANGED(SpellCasterComponent, cooldownSeconds,          PropertyDataType::Float, 0.0f, MaxSeconds),
				ADD_PROPERTY(SpellCasterComponent, chargeEffect,    PropertyDataType::String),
				ADD_PROPERTY_RANGED(SpellCasterComponent, chargeParticlesPerSecond, PropertyDataType::Float, 0.0f, MaxCharge),
				ADD_PROPERTY(SpellCasterComponent, castSound,       PropertyDataType::String),
				ADD_PROPERTY(SpellCasterComponent, castKey,         PropertyDataType::String),
				ADD_PROPERTY(SpellCasterComponent, displayName,     PropertyDataType::String),
				ADD_PROPERTY(SpellCasterComponent, isCastOnRequestOnly, PropertyDataType::Bool),
				ADD_PROPERTY(SpellCasterComponent, isUninterruptible, PropertyDataType::Bool)
			});
	}
};

}
