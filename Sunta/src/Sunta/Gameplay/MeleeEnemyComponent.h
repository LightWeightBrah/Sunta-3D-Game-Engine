#pragma once

#include "ECS/ComponentLayout.h"
#include "Core/EngineAssets.h"

namespace Sunta
{

// A kind of enemy: a sword fighter that chases the player, attacks up close, circles and gives up when the player gets away
//
// How to use it: Add Component > Melee Enemy (it adds the Enemy it is built on). Its states are
// Idle, Chase, Attack, Recover, ReturnHome (see MeleeEnemy.h), the numbers below tune them in the Inspector
struct MeleeEnemyComponent
{
	// Limits of the sliders in the Inspector
	static constexpr float MaxDistance = 100.0f;
	static constexpr float MaxSeconds  = 10.0f;

	// Close enough to swing, far enough to run, so far that it gives up and walks back home
	float attackDistance = 2.0f;
	float runDistance    = 6.0f;
	float giveUpDistance = 18.0f;

	// Time between two attacks, the chance (0..1) that an attack is a heavy one, how many light attacks it chains at most
	float attackCooldownSeconds = 1.4f;
	float heavyAttackChance     = 0.25f;
	int   maxLightCombo         = 2;

	// How long it circles the player after attacking before it comes in again
	float recoverSeconds = 1.0f;

	static void RegisterToInspector()
	{
		InspectorComponentRegistry::RegisterComponent<MeleeEnemyComponent>("Melee Enemy", EngineAssets::Icons::StateMachine,
			{
				ADD_PROPERTY_RANGED(MeleeEnemyComponent, attackDistance,        PropertyDataType::Float, 0.0f, MaxDistance),
				ADD_PROPERTY_RANGED(MeleeEnemyComponent, runDistance,           PropertyDataType::Float, 0.0f, MaxDistance),
				ADD_PROPERTY_RANGED(MeleeEnemyComponent, giveUpDistance,        PropertyDataType::Float, 0.0f, MaxDistance),
				ADD_PROPERTY_RANGED(MeleeEnemyComponent, attackCooldownSeconds, PropertyDataType::Float, 0.0f, MaxSeconds),
				ADD_PROPERTY_RANGED(MeleeEnemyComponent, heavyAttackChance,     PropertyDataType::Float, 0.0f, 1.0f),
				ADD_PROPERTY(MeleeEnemyComponent, maxLightCombo, PropertyDataType::Int),
				ADD_PROPERTY_RANGED(MeleeEnemyComponent, recoverSeconds,        PropertyDataType::Float, 0.0f, MaxSeconds)
			});
	}
};

}
