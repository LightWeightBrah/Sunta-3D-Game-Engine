#pragma once

#include <string>
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include "ECS/ComponentLayout.h"
#include "Core/EngineAssets.h"

namespace Sunta
{

// Thousands of copies of one model moving together: a cloud of missiles that bursts out of a point into a turning
// sphere, spreads over the whole sky, hangs there, then rains down glowing and stays stuck in the ground burning
// (a great spell, a storm of blades, a flock seen from afar). Every copy has its own size, turn and timing.
// All of them are drawn at once (instancing), so a few thousand cost about as much as one model
//
// How to use it: Add Component > Swarm on an entity where the cloud is born (its center), choose 'modelName' (a model
// without bones) and 'count'. A script moves it from stage to stage by changing the numbers below:
//   this.swarm.isVisible = true
//   this.swarm.gathered = 1     -- 0 = all in the center, 1 = all on the sphere of 'radius' (each leaves at its own moment)
//   this.swarm.radius = 30      -- a growing sphere
//   this.swarm.spread = 1       -- 0 = on the sphere, 1 = spread over the sky box ('skyCenter', 'skyHalfExtents')
//   this.swarm:fall()           -- they come down, one after another during 'fallSpreadSeconds'
//   this.swarm:reset()          -- back to the start, hidden
struct SwarmComponent
{
	// Limits of the sliders in the Inspector
	static constexpr int   MaxCount   = 6000;
	static constexpr float MaxSize    = 20.0f;
	static constexpr float MaxRadius  = 200.0f;
	static constexpr float MaxTurn    = 1440.0f;
	static constexpr float MaxSeconds = 30.0f;
	static constexpr float MaxSpeed   = 200.0f;

	bool isVisible = true;

	std::string modelName;
	int   count         = 500;
	int   seed          = 1;

	// The scale of each copy, some bigger and some smaller by up to this part
	float pieceSize     = 1.0f;
	float sizeVariation = 0.5f;

	// ========== The sphere around the entity ==========

	float radius         = 10.0f;
	// How deep the shell is, a part of the radius (0 = all on the surface, 1 = the whole ball is full)
	float shellThickness = 0.35f;
	// The whole cloud turns around the vertical axis, and each copy turns around an axis of its own
	float spinDegreesPerSecond   = 20.0f;
	float tumbleDegreesPerSecond = 90.0f;

	// ========== The sky they spread over (world space) ==========

	glm::vec3 skyCenter      = glm::vec3(0.0f, 30.0f, 0.0f);
	glm::vec3 skyHalfExtents = glm::vec3(40.0f, 8.0f, 40.0f);

	// ========== The fall ==========

	float fallSpeed         = 30.0f;
	// The first starts at once, the last after this long
	float fallSpreadSeconds = 4.0f;
	// Where the ground is (world height): they stop there, stuck, glowing, and fade after 'stuckSeconds'
	float groundHeight      = 0.0f;
	float stuckSeconds      = 4.0f;
	// The light of a hot copy (falling: it heats up; stuck: it cools down) and of the streak behind it (meters long)
	glm::vec3 glowColor     = glm::vec3(3.0f, 1.2f, 0.3f);
	float trailLength       = 6.0f;
	// Specks of the emitter named 'hitEffect' where each one lands
	std::string hitEffect;
	int   hitBurst          = 3;

	// ========== Set by a script, not saved with the scene ==========

	float gathered = 0.0f;
	float spread   = 0.0f;

	// ========== Filled while the game plays, not saved with the scene ==========

	struct Piece
	{
		glm::vec3 direction;     // on the sphere
		float     depth;         // into the shell, 0..1
		glm::vec3 skyOffset;     // in the sky box, -1..1
		float     size;
		glm::vec3 tumbleAxis;
		float     tumbleSpeed;   // degrees per second
		float     phase;         // degrees
		float     leaveAt;       // 0..1: when it leaves the center (gathered) and the sphere (spread)
		float     fallDelay;     // seconds after fall()
		glm::vec3 fallTilt;      // a falling copy comes down a little aslant

		// The fall
		bool      isFalling = false;
		bool      isStuck   = false;
		bool      isBroken  = false;   // struck by something after it stuck: gone (SwarmSystem::BreakWithin)
		glm::vec3 position  = glm::vec3(0.0f);
		glm::quat rotation  = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
		float     heat      = 0.0f;
		float     stuckFor  = 0.0f;
	};

	struct Instance
	{
		glm::mat4 matrix;
		glm::vec4 glow;          // rgb light, a unused
	};

	struct Streak
	{
		glm::vec4 head;          // xyz where it is, w its width
		glm::vec4 tail;          // xyz where it was, w how hot (0..1)
	};

	std::vector<Piece>    pieces;
	std::vector<Instance> instances;   // what is drawn this frame
	std::vector<Streak>   streaks;
	int   builtCount  = -1;
	int   builtSeed   = 0;
	float seconds     = 0.0f;
	float fallSeconds = -1.0f;         // < 0: not falling

	static void RegisterToInspector()
	{
		InspectorComponentRegistry::RegisterComponent<SwarmComponent>("Swarm", EngineAssets::Icons::Particle,
			{
				ADD_PROPERTY(SwarmComponent, isVisible, PropertyDataType::Bool),
				ADD_ASSET(SwarmComponent, modelName, AssetType::Model),
				ADD_PROPERTY_RANGED(SwarmComponent, count, PropertyDataType::Int, 1, MaxCount),
				ADD_PROPERTY(SwarmComponent, seed, PropertyDataType::Int),
				ADD_PROPERTY_RANGED(SwarmComponent, pieceSize,              PropertyDataType::Float, 0.0f, MaxSize),
				ADD_PROPERTY_RANGED(SwarmComponent, sizeVariation,          PropertyDataType::Float, 0.0f, 1.0f),
				ADD_PROPERTY_RANGED(SwarmComponent, radius,                 PropertyDataType::Float, 0.0f, MaxRadius),
				ADD_PROPERTY_RANGED(SwarmComponent, shellThickness,         PropertyDataType::Float, 0.0f, 1.0f),
				ADD_PROPERTY_RANGED(SwarmComponent, spinDegreesPerSecond,   PropertyDataType::Float, 0.0f, MaxTurn),
				ADD_PROPERTY_RANGED(SwarmComponent, tumbleDegreesPerSecond, PropertyDataType::Float, 0.0f, MaxTurn),
				ADD_PROPERTY(SwarmComponent, skyCenter,      PropertyDataType::Float3),
				ADD_PROPERTY(SwarmComponent, skyHalfExtents, PropertyDataType::Float3),
				ADD_PROPERTY_RANGED(SwarmComponent, fallSpeed,         PropertyDataType::Float, 0.0f, MaxSpeed),
				ADD_PROPERTY_RANGED(SwarmComponent, fallSpreadSeconds, PropertyDataType::Float, 0.0f, MaxSeconds),
				ADD_PROPERTY(SwarmComponent, groundHeight, PropertyDataType::Float),
				ADD_PROPERTY_RANGED(SwarmComponent, stuckSeconds,      PropertyDataType::Float, 0.0f, MaxSeconds),
				ADD_PROPERTY(SwarmComponent, glowColor, PropertyDataType::Color),
				ADD_PROPERTY_RANGED(SwarmComponent, trailLength,       PropertyDataType::Float, 0.0f, MaxRadius),
				ADD_PROPERTY(SwarmComponent, hitEffect, PropertyDataType::String),
				ADD_PROPERTY(SwarmComponent, hitBurst,  PropertyDataType::Int)
			});
	}
};

}
