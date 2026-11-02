#pragma once

#include <vector>
#include <glm/glm.hpp>

#include "ECS/ComponentLayout.h"
#include "Core/EngineAssets.h"

namespace Sunta
{

// A glowing streak behind a swung weapon: the blade leaves a fading ribbon of light along the path of its swing
//
// How to use it: Add Component > Weapon Trail on the weapon (a child entity of the character, like the Hitbox).
// The owner is the closest parent with Character Actions: the trail is drawn during the swing of its attacks (from just
// before the striking part until just after it), or always with 'onlyWhileStriking' off. 'baseOffset' and 'tipOffset'
// are where the blade starts and ends, in the space of the weapon (the Knight's sword: along +Y from the grip)
struct WeaponTrailComponent
{
	// Limits of the sliders in the Inspector
	static constexpr float MaxIntensity = 10.0f;
	static constexpr float MaxLifetime  = 1.0f;

	glm::vec3 baseOffset = glm::vec3(0.0f, 0.15f, 0.0f);
	glm::vec3 tipOffset  = glm::vec3(0.0f, 0.95f, 0.0f);

	// The light of the streak (its light is added to the picture, above 1 it glows in the bloom)
	glm::vec3 color     = glm::vec3(1.0f, 0.85f, 0.6f);
	float     intensity = 1.2f;

	// How long a piece of the streak takes to fade away (seconds): longer = a longer tail behind the blade
	float lifetimeSeconds = 0.16f;

	bool onlyWhileStriking = true;

	// ========== Filled while the game plays, not saved with the scene ==========

	// Where the blade was, newest last, and how long ago
	struct Sample
	{
		glm::vec3 base;
		glm::vec3 tip;
		float     age;
	};
	std::vector<Sample> samples;

	static void RegisterToInspector()
	{
		InspectorComponentRegistry::RegisterComponent<WeaponTrailComponent>("Weapon Trail", EngineAssets::Icons::Bonfire,
			{
				ADD_PROPERTY(WeaponTrailComponent, baseOffset, PropertyDataType::Float3),
				ADD_PROPERTY(WeaponTrailComponent, tipOffset,  PropertyDataType::Float3),
				ADD_PROPERTY(WeaponTrailComponent, color,      PropertyDataType::Color),
				ADD_PROPERTY_RANGED(WeaponTrailComponent, intensity,       PropertyDataType::Float, 0.0f, MaxIntensity),
				ADD_PROPERTY_RANGED(WeaponTrailComponent, lifetimeSeconds, PropertyDataType::Float, 0.02f, MaxLifetime),
				ADD_PROPERTY(WeaponTrailComponent, onlyWhileStriking, PropertyDataType::Bool)
			});
	}
};

}
