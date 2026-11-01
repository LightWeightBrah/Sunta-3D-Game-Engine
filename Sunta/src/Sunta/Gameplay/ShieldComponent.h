#pragma once

#include "ECS/ComponentLayout.h"
#include "Core/EngineAssets.h"

namespace Sunta
{

// A shield the character blocks with (the Dark Souls style of guarding)
//
// How to use it: put it on the shield, a child entity of the character (the Souls Player and the enemies get it
// on their left hand). The owner is the closest parent with Character Actions, which blocks while the block key is held
// (Left Ctrl) and parries with a tap (F)
//
// A hit that comes from the front (inside 'guardAngleDegrees') while the owner blocks hurts only by 'damageThrough'
// and costs stamina instead: 'staminaPerDamage' for every point of the blocked damage. When there is not enough stamina
// the guard breaks: the owner staggers and takes the whole hit
struct ShieldComponent
{
	// Limits of the sliders in the Inspector
	static constexpr float MaxGuardAngleDegrees = 360.0f;
	static constexpr float MaxStaminaPerDamage  = 5.0f;

	float guardAngleDegrees = 140.0f;

	// The part of the damage (0..1) that still hurts when the hit is blocked
	float damageThrough = 0.0f;

	float staminaPerDamage = 1.0f;

	static void RegisterToInspector()
	{
		InspectorComponentRegistry::RegisterComponent<ShieldComponent>("Shield", EngineAssets::Icons::BoxCollider,
			{
				ADD_PROPERTY_RANGED(ShieldComponent, guardAngleDegrees, PropertyDataType::Float, 0.0f, MaxGuardAngleDegrees),
				ADD_PROPERTY_RANGED(ShieldComponent, damageThrough,     PropertyDataType::Float, 0.0f, 1.0f),
				ADD_PROPERTY_RANGED(ShieldComponent, staminaPerDamage,  PropertyDataType::Float, 0.0f, MaxStaminaPerDamage)
			});
	}
};

}
