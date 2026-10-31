#pragma once

#include <glm/glm.hpp>

#include "ECS/ComponentLayout.h"
#include "Core/EngineAssets.h"

namespace Sunta
{

// Brings the player back after death: a few seconds after "YOU DIED" the player stands up at the last bonfire
// it rested at (at the start of the level when it rested nowhere yet), healed, and every enemy is back at its place
//
// How to use it: Add Component > Respawn on the player (the Souls Player has it), and put Bonfires in the level
struct RespawnComponent
{
	// Limit of the slider in the Inspector
	static constexpr float MaxDelaySeconds = 30.0f;

	// From the death to coming back
	float respawnDelaySeconds = 4.5f;

	// ========== Filled while the game plays, not saved with the scene ==========

	glm::vec3 respawnPosition = glm::vec3(0.0f);
	float     respawnYawDegrees = 0.0f;
	float     secondsDead = 0.0f;

	static void RegisterToInspector()
	{
		InspectorComponentRegistry::RegisterComponent<RespawnComponent>("Respawn", EngineAssets::Icons::SoulsBonfire,
			{
				ADD_PROPERTY_RANGED(RespawnComponent, respawnDelaySeconds, PropertyDataType::Float, 0.0f, MaxDelaySeconds)
			});
	}
};

}
