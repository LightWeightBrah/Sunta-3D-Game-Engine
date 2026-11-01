#pragma once

#include <string>
#include <utility>
#include <vector>
#include <glm/glm.hpp>

#include "ECS/ComponentLayout.h"
#include "Core/EngineAssets.h"

namespace Sunta
{

// A boss: an enemy that waits in an arena behind a fog gate (see FogGateComponent) and fights harder in its second phase
//
// How to use it: on an enemy (e.g. a Rusted Knight) together with Enemy Health Bar with 'isBoss' on (its name at the
// bottom of the screen). The boss wakes up the moment the fog closes behind the player, wherever the player is in the arena
// Below 'phaseTwoHealthFraction' of its health it attacks more often ('phaseTwoAttackSpeedup') and its weapon hurts more
// How the second phase starts can be shown too (all optional):
//  - 'phaseTwoAnimation': an animation of its Model played once (a roar), hits don't stagger it meanwhile
//  - 'phaseTwoGlow': the emissive strength of its Model and of its children's (the weapon) in phase two,
//    pulsing a little (models with an *_emissive texture start to glow: cracks with embers, red-hot metal). 0 = no change
//  - 'phaseTwoGrowth': it grows to this size (1.2 = 20% bigger) during the first seconds of phase two
//  - Particle Emitters on it or on its children start emitting (with a burst) in phase two
//  - a different set of attacks (Character Actions): the 'phaseTwo...Animation' names (empty = keep the old one), their hit
//    windows, a shockwave of heavy attacks and leaps ('phaseTwoShockwaveRadius', 0 = keep) and longer combos
// A defeated boss stays dead: resting at a bonfire or dying brings back every other enemy, not this one
//
// The component works with any kind of enemy: how the boss fights comes from its kind (Melee Enemy) or from Lua states
// (this.enemy:addState). A script can follow the fight and add its own phases: this.boss.phase (1 or 2), this.boss.isAwake,
// this.boss.isDefeated, e.g. if this.boss.phase == 2 then this.enemy:attack("heavy") end
struct BossComponent
{
	// Limits of the sliders in the Inspector
	static constexpr float MaxMultiplier = 5.0f;
	static constexpr float MaxGlow       = 10.0f;
	static constexpr float MaxGrowth     = 3.0f;
	static constexpr float MaxShockwaveRadius = 15.0f;

	float phaseTwoHealthFraction   = 0.5f;
	float phaseTwoDamageMultiplier = 1.3f;
	float phaseTwoAttackSpeedup    = 2.0f;

	std::string phaseTwoAnimation;
	float       phaseTwoGlow   = 0.0f;
	float       phaseTwoGrowth = 1.0f;

	std::string phaseTwoLight1Animation;
	std::string phaseTwoLight2Animation;
	std::string phaseTwoLight3Animation;
	std::string phaseTwoHeavyAnimation;
	float       phaseTwoLightHitFrom    = 0.45f;
	float       phaseTwoLightHitTo      = 0.65f;
	float       phaseTwoHeavyHitFrom    = 0.45f;
	float       phaseTwoHeavyHitTo      = 0.65f;
	float       phaseTwoShockwaveRadius = 0.0f;
	int         phaseTwoMaxLightCombo   = 0;   // 0 = keep

	// ========== Filled while the game plays, not saved with the scene ==========

	bool isAwake      = false;
	bool isInPhaseTwo = false;
	bool isDefeated   = false;

	// What the boss had before the fight changed it (given back when it is brought back)
	float restingHearingDistance = 0.0f;
	float restingAttackCooldown  = 0.0f;
	float restingWeaponDamage    = 0.0f;
	glm::vec3 restingScale       = glm::vec3(1.0f);

	// The emissive strength of the boss's model and its children's models before phase two: (entity, strength)
	std::vector<std::pair<unsigned int, float>> restingGlows;

	// The attacks of phase one, given back when the boss rests again
	std::string restingLight1Animation;
	std::string restingLight2Animation;
	std::string restingLight3Animation;
	std::string restingHeavyAnimation;
	float       restingLightHitFrom    = 0.0f;
	float       restingLightHitTo      = 0.0f;
	float       restingHeavyHitFrom    = 0.0f;
	float       restingHeavyHitTo      = 0.0f;
	float       restingShockwaveRadius = 0.0f;
	int         restingMaxLightCombo   = 0;

	// How long phase two lasts (it grows and glows from the start of it)
	float phaseTwoSeconds = 0.0f;

	static void RegisterToInspector()
	{
		InspectorComponentRegistry::RegisterComponent<BossComponent>("Boss", EngineAssets::Icons::StateMachine,
			{
				ADD_PROPERTY_RANGED(BossComponent, phaseTwoHealthFraction,   PropertyDataType::Float, 0.0f, 1.0f),
				ADD_PROPERTY_RANGED(BossComponent, phaseTwoDamageMultiplier, PropertyDataType::Float, 1.0f, MaxMultiplier),
				ADD_PROPERTY_RANGED(BossComponent, phaseTwoAttackSpeedup,    PropertyDataType::Float, 1.0f, MaxMultiplier),
				ADD_PROPERTY(BossComponent, phaseTwoAnimation, PropertyDataType::String),
				ADD_PROPERTY_RANGED(BossComponent, phaseTwoGlow,   PropertyDataType::Float, 0.0f, MaxGlow),
				ADD_PROPERTY_RANGED(BossComponent, phaseTwoGrowth, PropertyDataType::Float, 0.5f, MaxGrowth),
				ADD_PROPERTY(BossComponent, phaseTwoLight1Animation, PropertyDataType::String),
				ADD_PROPERTY(BossComponent, phaseTwoLight2Animation, PropertyDataType::String),
				ADD_PROPERTY(BossComponent, phaseTwoLight3Animation, PropertyDataType::String),
				ADD_PROPERTY(BossComponent, phaseTwoHeavyAnimation,  PropertyDataType::String),
				ADD_PROPERTY_RANGED(BossComponent, phaseTwoLightHitFrom, PropertyDataType::Float, 0.0f, 1.0f),
				ADD_PROPERTY_RANGED(BossComponent, phaseTwoLightHitTo,   PropertyDataType::Float, 0.0f, 1.0f),
				ADD_PROPERTY_RANGED(BossComponent, phaseTwoHeavyHitFrom, PropertyDataType::Float, 0.0f, 1.0f),
				ADD_PROPERTY_RANGED(BossComponent, phaseTwoHeavyHitTo,   PropertyDataType::Float, 0.0f, 1.0f),
				ADD_PROPERTY_RANGED(BossComponent, phaseTwoShockwaveRadius, PropertyDataType::Float, 0.0f, MaxShockwaveRadius),
				ADD_PROPERTY(BossComponent, phaseTwoMaxLightCombo, PropertyDataType::Int)
			});
	}
};

}
