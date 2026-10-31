#pragma once

#include <glm/glm.hpp>

#include "ECS/ComponentLayout.h"
#include "Core/EngineAssets.h"

namespace Sunta
{

// A place to rest (the Dark Souls bonfire)
//
// How to use it: Entity > Create Bonfire (or the toolbar) makes one with the model and a warm light. By hand: Add Component > Bonfire
// Near it the player sees "Rest at the bonfire [E]". Resting heals the player, refills the stamina, brings every enemy back
// to life at its starting place, and makes this bonfire the place the player comes back to after dying
// The player needs a Respawn component for that (the Souls Player has one)
struct BonfireComponent
{
	// Limit of the slider in the Inspector
	static constexpr float MaxInteractDistance = 10.0f;

	// How close the player must stand to rest (meters)
	float interactDistance = 2.2f;

	// Where the player stands up after dying, relative to the bonfire (meters, in the bonfire's own axes)
	glm::vec3 respawnOffset = glm::vec3(0.0f, 0.0f, 1.5f);

	// Set the first time the player rests here (then "BONFIRE LIT" is shown). Not saved with the scene
	bool isLit = false;

	static void RegisterToInspector()
	{
		InspectorComponentRegistry::RegisterComponent<BonfireComponent>("Bonfire", EngineAssets::Icons::SoulsBonfire,
			{
				ADD_PROPERTY_RANGED(BonfireComponent, interactDistance, PropertyDataType::Float, 0.0f, MaxInteractDistance),
				ADD_PROPERTY(BonfireComponent, respawnOffset, PropertyDataType::Float3)
			});
	}
};

}
