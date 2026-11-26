#pragma once

#include <string>

#include "ECS/ComponentLayout.h"
#include "Core/EngineAssets.h"

namespace Sunta
{

// The posture of a character, as in Sekiro: how long it can keep its guard against pressure before it breaks
//
// How to use it: Add Component > Posture on a character with Health (an enemy, a boss, the player). Its posture fills:
// - from every weapon blow that hurts it ('fromHits' x the blow's poise damage),
// - from blows it blocks ('fromBlocks' x the blocked damage, on top of the stamina the shield takes),
// - when its own blow is parried ('fromBeingParried', the deflect: the parrying one's whole point).
// It drains by itself 'recoverDelaySeconds' after the last fill, faster while the character is healthy (as in Sekiro:
// a hurt enemy keeps its posture broken longer). Full posture breaks: the character staggers for 'brokenSeconds'
// (the parried animation), and the first weapon blow from the front in that time is a finisher - 'finisherDamage' of
// its full health (1 kills it) - with slow motion, the finisher's effect and an event scripts see ("FINISHER")
// A character with Posture and no hits taken shows a posture bar over its health bar (the HUD) while it fills
struct PostureComponent
{
	// Limits of the sliders in the Inspector
	static constexpr float MaxPostureLimit  = 2000.0f;
	static constexpr float MaxSecondsLimit  = 30.0f;
	static constexpr float MaxFactorLimit   = 10.0f;

	float maxPosture = 100.0f;

	// Posture filled by the poise damage of a blow that hurts, by every point of the damage a shield takes,
	// and (a fixed amount) when the character's own blow is parried
	float fromHits         = 1.0f;
	float fromBlocks       = 0.6f;
	float fromBeingParried = 30.0f;

	// How fast it drains (points per second) once 'recoverDelaySeconds' passed since the last fill. At low health it
	// drains as slowly as 'lowHealthRecoverFactor' x this (1 = health doesn't matter)
	float recoverPerSecond       = 12.0f;
	float recoverDelaySeconds    = 1.5f;
	float lowHealthRecoverFactor = 0.35f;

	// How long a broken posture leaves the character open to the finisher
	float brokenSeconds = 2.5f;

	// The finisher: a part of the full health (1 = it dies; a boss with phases can take a few), and how much the
	// finishing blow slows the world (0 = not at all) for how long
	float finisherDamage        = 1.0f;
	float finisherSlowMotion    = 0.25f;
	float finisherSlowSeconds   = 0.6f;

	// Burst at the broken character when its posture breaks and when the finisher lands: names of entities with a
	// Particle Emitter (empty: nothing)
	std::string breakEffect;
	std::string finisherEffect;

	// ========== Filled while the game plays, not saved with the scene ==========

	float currentPosture   = 0.0f;
	float secondsSinceFill = 1000.0f;
	float brokenLeft       = 0.0f;
	// Times its posture broke and times it took a finisher (scripts count phases with them)
	int   breaks           = 0;
	int   finishers        = 0;

	bool IsBroken() const { return brokenLeft > 0.0f; }

	float GetFraction() const { return maxPosture > 0.0f ? currentPosture / maxPosture : 0.0f; }

	static void RegisterToInspector()
	{
		InspectorComponentRegistry::RegisterComponent<PostureComponent>("Posture", EngineAssets::Icons::Bonfire,
			{
				ADD_PROPERTY_RANGED(PostureComponent, maxPosture,             PropertyDataType::Float, 1.0f, MaxPostureLimit),
				ADD_PROPERTY_RANGED(PostureComponent, fromHits,               PropertyDataType::Float, 0.0f, MaxFactorLimit),
				ADD_PROPERTY_RANGED(PostureComponent, fromBlocks,             PropertyDataType::Float, 0.0f, MaxFactorLimit),
				ADD_PROPERTY_RANGED(PostureComponent, fromBeingParried,       PropertyDataType::Float, 0.0f, MaxPostureLimit),
				ADD_PROPERTY_RANGED(PostureComponent, recoverPerSecond,       PropertyDataType::Float, 0.0f, MaxPostureLimit),
				ADD_PROPERTY_RANGED(PostureComponent, recoverDelaySeconds,    PropertyDataType::Float, 0.0f, MaxSecondsLimit),
				ADD_PROPERTY_RANGED(PostureComponent, lowHealthRecoverFactor, PropertyDataType::Float, 0.0f, 1.0f),
				ADD_PROPERTY_RANGED(PostureComponent, brokenSeconds,          PropertyDataType::Float, 0.0f, MaxSecondsLimit),
				ADD_PROPERTY_RANGED(PostureComponent, finisherDamage,         PropertyDataType::Float, 0.0f, 1.0f),
				ADD_PROPERTY_RANGED(PostureComponent, finisherSlowMotion,     PropertyDataType::Float, 0.0f, 1.0f),
				ADD_PROPERTY_RANGED(PostureComponent, finisherSlowSeconds,    PropertyDataType::Float, 0.0f, MaxSecondsLimit),
				ADD_PROPERTY(PostureComponent, breakEffect,    PropertyDataType::String),
				ADD_PROPERTY(PostureComponent, finisherEffect, PropertyDataType::String)
			});
	}
};

}
