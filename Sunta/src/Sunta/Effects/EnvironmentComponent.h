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
	static constexpr int   MaxPixelSize      = 16;

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
	float bloomThreshold          = 0.7f;
	float bloomIntensity          = 1.2f;
	int   bloomBlurPasses         = 6;

	// How hard the player's blows (given and taken) kick the picture: an impact frame (see PostProcessing::Kick)
	// of the blow's camera shake times this. 0: never
	float hitImpactFrame          = 0.0f;

	// Pixel art made of 3D (the look of games drawn in low resolution with few colors): every 'pixelSize' x
	// 'pixelSize' screen pixels show one pixel of the picture (1 = off); 'paletteLevels' colors per channel
	// (0 = off), mixed in a fixed pattern ('ditherStrength' 0..1, the checkered blend of old games where two colors
	// meet); 'outlineStrength' 0..1 darkens the edges of things against what is behind them (from the depth),
	// 'outlineColor' is that dark ink
	int       pixelSize       = 1;
	int       paletteLevels   = 0;
	float     ditherStrength  = 0.5f;
	float     outlineStrength = 0.0f;
	glm::vec3 outlineColor    = glm::vec3(0.05f, 0.04f, 0.06f);

	// A painted look (the picture as an oil painting: flat strokes of color with sharp edges between them, which
	// hides how simple the models and their textures are): 'paintRadius' is the size of a stroke in screen pixels
	// (a Kuwahara filter; 0 = off). The grade of the colors: 'saturation' (1 = as lit), 'contrast' (1 = as lit),
	// 'colorTint' multiplies the picture (a warm gold of an evening), 'vignette' 0..1 darkens its corners
	int       paintRadius     = 0;
	float     saturation      = 1.0f;
	float     contrast        = 1.0f;
	glm::vec3 colorTint       = glm::vec3(1.0f);
	float     vignette        = 0.0f;

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
				ADD_PROPERTY(EnvironmentComponent, bloomBlurPasses, PropertyDataType::Int),
				ADD_PROPERTY_RANGED(EnvironmentComponent, hitImpactFrame, PropertyDataType::Float, 0.0f, 2.0f),
				ADD_PROPERTY_RANGED(EnvironmentComponent, pixelSize, PropertyDataType::Int, 1, MaxPixelSize),
				ADD_PROPERTY_RANGED(EnvironmentComponent, paletteLevels, PropertyDataType::Int, 0, 64),
				ADD_PROPERTY_RANGED(EnvironmentComponent, ditherStrength, PropertyDataType::Float, 0.0f, 1.0f),
				ADD_PROPERTY_RANGED(EnvironmentComponent, outlineStrength, PropertyDataType::Float, 0.0f, 1.0f),
				ADD_PROPERTY(EnvironmentComponent, outlineColor, PropertyDataType::Color),
				ADD_PROPERTY_RANGED(EnvironmentComponent, paintRadius, PropertyDataType::Int, 0, 8),
				ADD_PROPERTY_RANGED(EnvironmentComponent, saturation, PropertyDataType::Float, 0.0f, 3.0f),
				ADD_PROPERTY_RANGED(EnvironmentComponent, contrast, PropertyDataType::Float, 0.0f, 3.0f),
				ADD_PROPERTY(EnvironmentComponent, colorTint, PropertyDataType::Color),
				ADD_PROPERTY_RANGED(EnvironmentComponent, vignette, PropertyDataType::Float, 0.0f, 1.0f)
			});
	}
};

}
