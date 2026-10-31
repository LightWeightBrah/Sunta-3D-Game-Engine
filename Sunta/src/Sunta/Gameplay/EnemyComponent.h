#pragma once

#include <memory>
#include <glm/glm.hpp>

#include "ECS/ComponentLayout.h"
#include "Core/EngineAssets.h"

namespace Sunta
{

class EnemyStateMachine;

// What every enemy has, whatever kind it is: its senses and its state machine
//
// The kind of the enemy is a second component next to this one (Melee Enemy: a sword fighter, more kinds later),
// it brings the states that make the behaviour. Without a kind the enemy only has the basic states:
// Idle (stands still) and Dead (see BaseEnemyStates.h)
//
// A Lua script can add its own states or replace any state by name, so a new behaviour doesn't need C++:
//     this.enemy:addState("Idle", { update = function(enemy, deltaTime) ... return "Chase" end })
// (see the Enemy bindings in LuaBindings.cpp for everything a state can ask and do)
//
// Needs a Character Controller that is not player controlled (the enemy walks with it), Character Actions (to attack)
// and Lock On (to face the player). Add Component > Enemy adds the missing ones
struct EnemyComponent
{
	// Limits of the sliders in the Inspector
	static constexpr float MaxDistance = 100.0f;

	// What the enemy notices: the player in front of it within 'sightDistance' (and not behind a wall),
	// or anywhere closer than 'hearingDistance'
	float sightDistance     = 12.0f;
	float sightAngleDegrees = 75.0f; // to each side of where it looks
	float hearingDistance   = 3.0f;
	float eyeHeight         = 1.5f;

	// ========== Filled while the game plays, not saved with the scene ==========

	std::shared_ptr<EnemyStateMachine> stateMachine;

	// Where it stood when the game started (it walks back there when it loses the player)
	glm::vec3 homePosition = glm::vec3(0.0f);

	// Time left until the next attack is allowed (set by the states of the enemy's kind)
	float attackCooldownLeft = 0.0f;

	static void RegisterToInspector()
	{
		InspectorComponentRegistry::RegisterComponent<EnemyComponent>("Enemy", EngineAssets::Icons::StateMachine,
			{
				ADD_PROPERTY_RANGED(EnemyComponent, sightDistance,     PropertyDataType::Float, 0.0f, MaxDistance),
				ADD_PROPERTY_RANGED(EnemyComponent, sightAngleDegrees, PropertyDataType::Float, 0.0f, 180.0f),
				ADD_PROPERTY_RANGED(EnemyComponent, hearingDistance,   PropertyDataType::Float, 0.0f, MaxDistance),
				ADD_PROPERTY_RANGED(EnemyComponent, eyeHeight,         PropertyDataType::Float, 0.0f, 5.0f)
			});
	}
};

}
