#pragma once

#include <string>
#include <glm/glm.hpp>

#include "ECS/ComponentLayout.h"
#include "Core/EngineAssets.h"

namespace Sunta
{

// The fog in the doorway of a boss arena (as in Dark Souls)
//
// How to use it: on an entity standing in the doorway, with a Model (the fog) and a Box Collider that fills the doorway
// The arena is the box 'arenaCenter' +- 'arenaHalfExtents' (in the world). With 'isCrossedWithKey' (as in Dark Souls)
// the fog is a wall: standing close to it the player presses E and walks through it into the arena. Without it the fog
// can simply be walked through (its collider is a trigger). Once the player is inside the arena while a boss (BossComponent) is alive in it,
// the fog becomes a wall and the boss wakes up. When every boss of the arena is dead the fog disappears for good
// When the player dies, it comes back outside: the fog is passable again and the boss waits for the next try
// Particle Emitters on the fog or its children (swirling mist) stop when the fog disappears
// A Lua script can read the fight: this.fogGate.isClosed, this.fogGate.isCleared
struct FogGateComponent
{
	// Limit of the drag fields in the Inspector
	static constexpr float MaxArenaSize = 500.0f;

	glm::vec3 arenaCenter      = glm::vec3(0.0f, 0.0f, -40.0f);
	glm::vec3 arenaHalfExtents = glm::vec3(20.0f, 6.0f, 20.0f);

	// Shown in the middle of the screen when the last boss of the arena falls. Empty: nothing is shown
	std::string victoryMessage = "GREAT ENEMY FELLED";

	bool isCrossedWithKey = true;

	// ========== Filled while the game plays, not saved with the scene ==========

	bool isClosed  = false;
	bool isCleared = false;

	// Walking through the fog after E: how long it has been going on (below 0: nobody is crossing), from where to where
	float     crossingSeconds = -1.0f;
	glm::vec3 crossingFrom    = glm::vec3(0.0f);
	glm::vec3 crossingTo      = glm::vec3(0.0f);

	static void RegisterToInspector()
	{
		InspectorComponentRegistry::RegisterComponent<FogGateComponent>("Fog Gate", EngineAssets::Icons::BoxCollider,
			{
				ADD_PROPERTY(FogGateComponent, arenaCenter,      PropertyDataType::Float3),
				ADD_PROPERTY(FogGateComponent, arenaHalfExtents, PropertyDataType::Float3),
				ADD_PROPERTY(FogGateComponent, victoryMessage,   PropertyDataType::String),
				ADD_PROPERTY(FogGateComponent, isCrossedWithKey, PropertyDataType::Bool)
			});
	}
};

}
