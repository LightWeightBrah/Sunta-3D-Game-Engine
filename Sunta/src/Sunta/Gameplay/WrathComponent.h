#pragma once

#include <string>
#include <glm/glm.hpp>

#include "ECS/ComponentLayout.h"
#include "Core/EngineAssets.h"

namespace Sunta
{

// Wrath, a rage that grows in a fight (as the Spartan Rage of God of War): it fills from the damage the character deals
// and takes and from the blows it deflects; once full, the player sets it loose (V) - for 'durationSeconds' its blows
// hurt 'damageMultiplier' times more and fill the enemies' posture 'postureMultiplier' times faster, the figure burns
// in 'glowColor' (its outline and an aura). It drains away slowly when the fighting stops
//
// How to use it: Add Component > Wrath on the player (with Health and Character Actions). The HUD shows a thin bar
// under the stamina while it fills, pulsing when full. A script reads it: this.wrath.current, this.wrath.isActive
struct WrathComponent
{
	// Limits of the sliders in the Inspector
	static constexpr float MaxWrathLimit   = 1000.0f;
	static constexpr float MaxSecondsLimit = 60.0f;

	float maxWrath = 100.0f;

	// What fills it: every point of damage dealt with a weapon, every point taken, every deflected blow
	float fromDamageDealt = 0.5f;
	float fromDamageTaken = 0.3f;
	float fromDeflects    = 8.0f;

	// It drains this much a second after 'drainDelaySeconds' without filling (0 = it keeps what it has)
	float drainPerSecond    = 2.0f;
	float drainDelaySeconds = 6.0f;

	// Set loose: how long, how much harder the blows land and how much faster they break a guard
	float durationSeconds   = 8.0f;
	float damageMultiplier  = 1.4f;
	float postureMultiplier = 1.5f;

	// The figure while it burns (its rim and an aura around it) and what bursts when it is set loose (an entity with
	// a Particle Emitter; empty = nothing)
	glm::vec3   glowColor = glm::vec3(0.75f, 0.35f, 1.0f);
	std::string releaseEffect;

	// ========== Filled while the game plays, not saved with the scene ==========

	float currentWrath     = 0.0f;
	float activeLeft       = 0.0f;
	float secondsSinceFill = 1000.0f;

	bool  IsActive() const { return activeLeft > 0.0f; }
	bool  IsFull()   const { return currentWrath >= maxWrath; }
	float GetFraction() const { return maxWrath > 0.0f ? currentWrath / maxWrath : 0.0f; }

	static void RegisterToInspector()
	{
		InspectorComponentRegistry::RegisterComponent<WrathComponent>("Wrath", EngineAssets::Icons::Bonfire,
			{
				ADD_PROPERTY_RANGED(WrathComponent, maxWrath,          PropertyDataType::Float, 1.0f, MaxWrathLimit),
				ADD_PROPERTY_RANGED(WrathComponent, fromDamageDealt,   PropertyDataType::Float, 0.0f, 10.0f),
				ADD_PROPERTY_RANGED(WrathComponent, fromDamageTaken,   PropertyDataType::Float, 0.0f, 10.0f),
				ADD_PROPERTY_RANGED(WrathComponent, fromDeflects,      PropertyDataType::Float, 0.0f, 100.0f),
				ADD_PROPERTY_RANGED(WrathComponent, drainPerSecond,    PropertyDataType::Float, 0.0f, 100.0f),
				ADD_PROPERTY_RANGED(WrathComponent, drainDelaySeconds, PropertyDataType::Float, 0.0f, MaxSecondsLimit),
				ADD_PROPERTY_RANGED(WrathComponent, durationSeconds,   PropertyDataType::Float, 0.1f, MaxSecondsLimit),
				ADD_PROPERTY_RANGED(WrathComponent, damageMultiplier,  PropertyDataType::Float, 1.0f, 5.0f),
				ADD_PROPERTY_RANGED(WrathComponent, postureMultiplier, PropertyDataType::Float, 1.0f, 5.0f),
				ADD_PROPERTY(WrathComponent, glowColor, PropertyDataType::Color),
				ADD_PROPERTY(WrathComponent, releaseEffect, PropertyDataType::String)
			});
	}
};

}
