#pragma once

#include <string>
#include <vector>

#include <glm/glm.hpp>

#include "ECS/ComponentLayout.h"
#include "Core/EngineAssets.h"

namespace Sunta
{

// The ground of an open world: hills, valleys and roads from a heightmap (a grey picture: black is the lowest,
// white the highest). Bodies (the player, enemies, a horse) stand on it and walk up and down its slopes
//
// How to use it: Add Component > Terrain on an entity whose Model is the terrain's mesh made from the same picture
// (e.g. Tools/Blender: a grid displaced by the heightmap). 'heightmap' is a .png (16-bit for smooth slopes),
// 'sizeX' / 'sizeZ' its width and depth in meters, centered on the entity's position; 'heightScale' the height of white
// above the entity. Outside the picture there is no ground from it (box colliders still are)
struct TerrainComponent
{
	static constexpr float MaxSize   = 4000.0f;
	static constexpr float MaxHeight = 500.0f;

	std::string heightmap;
	float       sizeX       = 100.0f;
	float       sizeZ       = 100.0f;
	float       heightScale = 10.0f;

	// ========== Read when the game starts, not saved with the scene ==========

	std::vector<float> heights;  // 0..1, row by row (rows along Z, from -Z to +Z of the area)
	int                columns = 0;
	int                rows    = 0;
	std::string        loadedPath;

	static void RegisterToInspector()
	{
		InspectorComponentRegistry::RegisterComponent<TerrainComponent>("Terrain", EngineAssets::Icons::Transform,
			{
				ADD_PROPERTY(TerrainComponent, heightmap, PropertyDataType::String),
				ADD_PROPERTY_RANGED(TerrainComponent, sizeX, PropertyDataType::Float, 1.0f, MaxSize),
				ADD_PROPERTY_RANGED(TerrainComponent, sizeZ, PropertyDataType::Float, 1.0f, MaxSize),
				ADD_PROPERTY_RANGED(TerrainComponent, heightScale, PropertyDataType::Float, 0.0f, MaxHeight)
			});
	}
};

}
