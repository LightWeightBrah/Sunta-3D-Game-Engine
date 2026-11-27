#pragma once

#include <string>
#include <vector>
#include <glm/glm.hpp>

#include "ECS/ComponentLayout.h"
#include "Core/EngineAssets.h"

namespace Sunta
{

// Grass, flowers, stones: thousands of copies of a small model scattered over an area and drawn in one call
//
// How to use it: Add Component > Foliage on an entity in the middle of the area. 'modelName' is a model without bones
// (a tuft of grass made of crossed cards with an alpha texture: transparent pixels are cut out); 'count' copies are
// scattered over the 'areaSizeX' x 'areaSizeZ' rectangle around the entity, each turned at random, between 'minScale'
// and 'maxScale' big, standing on a Terrain where there is one. A 'densityMask' (a grey picture over the area: white =
// full, black = none, rows from -Z to +Z) keeps them off roads and out of camps. The wind sways their tops ('sway'
// meters at the top of a 1 m model, 'swaySpeed'); past 'drawDistance' meters from the camera they shrink away (no
// popping). 'tint' colors them, 'tintVariation' 0..1 makes some darker or lighter
struct FoliageComponent
{
	static constexpr float MaxArea     = 4000.0f;
	static constexpr int   MaxCount    = 200000;
	static constexpr float MaxDistance = 500.0f;

	std::string modelName;
	int         count = 2000;
	float       areaSizeX = 50.0f;
	float       areaSizeZ = 50.0f;
	std::string densityMask;
	float       minScale = 0.7f;
	float       maxScale = 1.3f;
	float       sway      = 0.08f;
	float       swaySpeed = 1.6f;
	float       drawDistance = 60.0f;
	glm::vec3   tint = glm::vec3(1.0f);
	float       tintVariation = 0.25f;
	int         seed = 1;

	// ========== Filled while the game plays, not saved with the scene ==========

	// Every copy: its place (xyz) and size (w), its turn (x, radians), its tint factor (y), its sway phase (z)
	std::vector<glm::vec4> places;
	std::vector<glm::vec4> extras;
	bool isScattered = false;

	static void RegisterToInspector()
	{
		InspectorComponentRegistry::RegisterComponent<FoliageComponent>("Foliage", EngineAssets::Icons::Bonfire,
			{
				ADD_PROPERTY(FoliageComponent, modelName, PropertyDataType::String),
				ADD_PROPERTY_RANGED(FoliageComponent, count, PropertyDataType::Int, 0, MaxCount),
				ADD_PROPERTY_RANGED(FoliageComponent, areaSizeX, PropertyDataType::Float, 1.0f, MaxArea),
				ADD_PROPERTY_RANGED(FoliageComponent, areaSizeZ, PropertyDataType::Float, 1.0f, MaxArea),
				ADD_PROPERTY(FoliageComponent, densityMask, PropertyDataType::String),
				ADD_PROPERTY_RANGED(FoliageComponent, minScale, PropertyDataType::Float, 0.01f, 20.0f),
				ADD_PROPERTY_RANGED(FoliageComponent, maxScale, PropertyDataType::Float, 0.01f, 20.0f),
				ADD_PROPERTY_RANGED(FoliageComponent, sway, PropertyDataType::Float, 0.0f, 2.0f),
				ADD_PROPERTY_RANGED(FoliageComponent, swaySpeed, PropertyDataType::Float, 0.0f, 10.0f),
				ADD_PROPERTY_RANGED(FoliageComponent, drawDistance, PropertyDataType::Float, 1.0f, MaxDistance),
				ADD_PROPERTY(FoliageComponent, tint, PropertyDataType::Color),
				ADD_PROPERTY_RANGED(FoliageComponent, tintVariation, PropertyDataType::Float, 0.0f, 1.0f),
				ADD_PROPERTY(FoliageComponent, seed, PropertyDataType::Int)
			});
	}
};

}
