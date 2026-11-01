#pragma once

#include "ECS/ComponentLayout.h"
#include "Core/EngineAssets.h"

namespace Sunta
{

// A boss: an enemy that waits in an arena behind a fog gate (see FogGateComponent) and fights harder in its second phase
//
// How to use it: on an enemy (e.g. a Rusted Knight) together with Enemy Health Bar with 'isBoss' on (its name at the
// bottom of the screen). The boss wakes up the moment the fog closes behind the player, wherever the player is in the arena
// Below 'phaseTwoHealthFraction' of its health it attacks more often ('phaseTwoAttackSpeedup') and its weapon hurts more
// A defeated boss stays dead: resting at a bonfire or dying brings back every other enemy, not this one
//
// The component works with any kind of enemy: how the boss fights comes from its kind (Melee Enemy) or from Lua states
// (this.enemy:addState). A script can follow the fight and add its own phases: this.boss.phase (1 or 2), this.boss.isAwake,
// this.boss.isDefeated, e.g. if this.boss.phase == 2 then this.enemy:attack("heavy") end
struct BossComponent
{
	// Limits of the sliders in the Inspector
	static constexpr float MaxMultiplier = 5.0f;

	float phaseTwoHealthFraction   = 0.5f;
	float phaseTwoDamageMultiplier = 1.3f;
	float phaseTwoAttackSpeedup    = 2.0f;

	// ========== Filled while the game plays, not saved with the scene ==========

	bool isAwake      = false;
	bool isInPhaseTwo = false;
	bool isDefeated   = false;

	// What the boss had before the fight changed it (given back when it is brought back)
	float restingHearingDistance = 0.0f;
	float restingAttackCooldown  = 0.0f;
	float restingWeaponDamage    = 0.0f;

	static void RegisterToInspector()
	{
		InspectorComponentRegistry::RegisterComponent<BossComponent>("Boss", EngineAssets::Icons::StateMachine,
			{
				ADD_PROPERTY_RANGED(BossComponent, phaseTwoHealthFraction,   PropertyDataType::Float, 0.0f, 1.0f),
				ADD_PROPERTY_RANGED(BossComponent, phaseTwoDamageMultiplier, PropertyDataType::Float, 1.0f, MaxMultiplier),
				ADD_PROPERTY_RANGED(BossComponent, phaseTwoAttackSpeedup,    PropertyDataType::Float, 1.0f, MaxMultiplier)
			});
	}
};

}
