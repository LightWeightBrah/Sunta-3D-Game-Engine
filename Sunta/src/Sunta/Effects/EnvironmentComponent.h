#pragma once

#include <string>
#include <glm/glm.hpp>

#include "ECS/ComponentLayout.h"
#include "Core/EngineAssets.h"

namespace Sunta
{

// The look of the whole scene: the sky around it, the fog in it, and how the picture is put on the screen (HDR, bloom)
//
// How to use it: Add Component > Environment on one entity of the scene (e.g. the Moonlight); a second one is ignored
// 'skyboxFolder' is a folder with six pictures of the sky: right, left, top, bottom, front and back (.jpg or .png),
// e.g. @game/Skyboxes/AshenNight (Tools/Skybox/panorama_to_cubemap.py makes them from a panorama). Empty = no sky
// 'skyBrightness' darkens or brightens it. With 'isFogEnabled' far things fade into 'fogColor': 'fogDensity' 0.03
// is a light haze (half the color at about 28 m), 0.08 a thick fog (half at about 10 m)
// A script can change it during the game: this.environment.fogDensity = 0.1 (the fog thickens when the boss wakes up)
struct EnvironmentComponent
{
	// Limits of the sliders in the Inspector
	static constexpr float MaxSkyBrightness = 5.0f;
	static constexpr float MaxFogDensity    = 0.5f;
	static constexpr float MaxExposure       = 4.0f;
	static constexpr float MaxBloomThreshold = 5.0f;
	static constexpr float MaxBloomIntensity = 3.0f;

	std::string skyboxFolder;
	float       skyBrightness = 1.0f;

	bool      isFogEnabled = true;
	glm::vec3 fogColor     = glm::vec3(0.08f, 0.075f, 0.075f);
	float     fogDensity   = 0.03f;

	// HDR (see PostProcessing): the scene keeps lights brighter than white and is tone mapped onto the screen
	// 'exposure' brightens or darkens the whole picture. Bloom: what is brighter than 'bloomThreshold' glows around
	// itself, 'bloomIntensity' strong, 'bloomBlurPasses' wide. Without an Environment the defaults below are used
	bool  isPostProcessingEnabled = true;
	float exposure                = 0.8f;
	bool  isBloomEnabled          = true;
	float bloomThreshold          = 1.0f;
	float bloomIntensity          = 0.6f;
	int   bloomBlurPasses         = 5;

	static void RegisterToInspector()
	{
		InspectorComponentRegistry::RegisterComponent<EnvironmentComponent>("Environment", EngineAssets::Icons::Bonfire,
			{
				ADD_PROPERTY(EnvironmentComponent, skyboxFolder, PropertyDataType::String),
				ADD_PROPERTY_RANGED(EnvironmentComponent, skyBrightness, PropertyDataType::Float, 0.0f, MaxSkyBrightness),
				ADD_PROPERTY(EnvironmentComponent, isFogEnabled, PropertyDataType::Bool),
				ADD_PROPERTY(EnvironmentComponent, fogColor, PropertyDataType::Color),
				ADD_PROPERTY_RANGED(EnvironmentComponent, fogDensity, PropertyDataType::Float, 0.0f, MaxFogDensity),
				ADD_PROPERTY(EnvironmentComponent, isPostProcessingEnabled, PropertyDataType::Bool),
				ADD_PROPERTY_RANGED(EnvironmentComponent, exposure, PropertyDataType::Float, 0.0f, MaxExposure),
				ADD_PROPERTY(EnvironmentComponent, isBloomEnabled, PropertyDataType::Bool),
				ADD_PROPERTY_RANGED(EnvironmentComponent, bloomThreshold, PropertyDataType::Float, 0.0f, MaxBloomThreshold),
				ADD_PROPERTY_RANGED(EnvironmentComponent, bloomIntensity, PropertyDataType::Float, 0.0f, MaxBloomIntensity),
				ADD_PROPERTY(EnvironmentComponent, bloomBlurPasses, PropertyDataType::Int)
			});
	}
};

}
