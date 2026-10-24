#pragma once

#include <nlohmann/json.hpp>

#include "Physics/CollisionSettings.h"

namespace Sunta
{

// How the collision settings look inside game.config:
//   "collision_layers": { "Environment": 0, "Player": 1 }       layer name -> slot (the bit scene files store)
//   "collision_matrix": [ "Environment + Player", ... ]         every pair of layers that collide, written once
class CollisionSettingsSerializer
{
public:
	// Forgiving: a config that has neither key (an older project) gets the fallback
	static CollisionSettings Deserialize(const nlohmann::json& configJson, const CollisionSettings& fallback);

	// Adds both keys to the config
	static void Serialize(nlohmann::ordered_json& configJson, const CollisionSettings& settings);
};

}
