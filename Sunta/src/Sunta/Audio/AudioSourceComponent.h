#pragma once

#include <string>

#include "ECS/ComponentLayout.h"
#include "Core/EngineAssets.h"

namespace Sunta
{

// A sound that belongs to an entity: a bonfire crackling, wind over the ruins, a chant in a chapel
//
// How to use it: Add Component > Audio Source, 'sound' = a file or a folder of files (@game/Sounds/Ambience/fire.ogg;
// a folder plays one of them at random). With 'playOnStart' it starts with the game, with 'isLooping' it never ends
// A spatial source ('isSpatial') is heard from the entity and follows it: full 'volume' up to 'minDistance' meters,
// quieter further away, no quieter beyond 'maxDistance'. Without it the sound is the same everywhere (wind, music)
// A script can play it: this.audioSource:play(), this.audioSource:stop(), this.audioSource.volume = 0.5
struct AudioSourceComponent
{
	// Limits of the sliders in the Inspector
	static constexpr float MaxVolume   = 2.0f;
	static constexpr float MaxDistance = 200.0f;

	std::string sound;
	float       volume      = 1.0f;
	bool        isLooping   = true;
	bool        playOnStart = true;

	bool  isSpatial   = true;
	float minDistance = 2.0f;
	float maxDistance = 30.0f;

	// ========== Filled while the game plays, not saved with the scene ==========

	unsigned int voice = 0;

	static void RegisterToInspector()
	{
		InspectorComponentRegistry::RegisterComponent<AudioSourceComponent>("Audio Source", EngineAssets::Icons::AudioFile,
			{
				ADD_PROPERTY(AudioSourceComponent, sound, PropertyDataType::String),
				ADD_PROPERTY_RANGED(AudioSourceComponent, volume, PropertyDataType::Float, 0.0f, MaxVolume),
				ADD_PROPERTY(AudioSourceComponent, isLooping, PropertyDataType::Bool),
				ADD_PROPERTY(AudioSourceComponent, playOnStart, PropertyDataType::Bool),
				ADD_PROPERTY(AudioSourceComponent, isSpatial, PropertyDataType::Bool),
				ADD_PROPERTY_RANGED(AudioSourceComponent, minDistance, PropertyDataType::Float, 0.0f, MaxDistance),
				ADD_PROPERTY_RANGED(AudioSourceComponent, maxDistance, PropertyDataType::Float, 0.0f, MaxDistance)
			});
	}
};

}
