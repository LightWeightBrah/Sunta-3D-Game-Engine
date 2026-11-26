#pragma once

#include <string>

#include <glm/glm.hpp>

#include "ECS/ComponentLayout.h"
#include "Core/EngineAssets.h"

namespace Sunta
{

// A mount the player can ride (a horse, a beast of burden, a war wolf): the open-world way to travel
//
// How to use it: Add Component > Mount on a character that has a Character Controller (turn its 'isPlayerControlled'
// off: the mount moves only when ridden), a Model with its gaits and a Character Animation (idle / walk / run are the
// mount's stand, walk and gallop). Next to it the player sees 'mountPrompt' and mounts with E; while riding W A S D
// steer it (relative to the camera), Left Shift gallops, E again dismounts beside it
// The rider sits 'seatOffset' meters from the mount's feet (in the mount's own space: +Y up, -Z its front), turned
// with it, and plays 'riderIdleAnimation' / 'riderWalkAnimation' / 'riderRunAnimation' by the mount's speed. Its own
// keys and attacks wait until it dismounts. The camera pivot rises by 'cameraRise' and the camera stands
// 'cameraPullBack' meters farther while riding
struct MountComponent
{
	// Limits of the sliders in the Inspector
	static constexpr float MaxDistance = 10.0f;
	static constexpr float MaxRise     = 5.0f;

	glm::vec3 seatOffset = glm::vec3(0.0f, 1.2f, 0.1f);

	float mountDistance = 2.6f;

	std::string mountPrompt    = "Mount   [E]";
	std::string dismountPrompt = "Dismount   [E]";

	std::string riderIdleAnimation = "Ride_Idle";
	std::string riderWalkAnimation = "Ride_Walk";
	std::string riderRunAnimation  = "Ride_Gallop";

	// Above this speed (m/s) the rider plays the gallop
	float runAnimationFromSpeed = 4.5f;

	float cameraRise = 1.1f;
	// ...and the camera stands this much farther back (the mount fills the view)
	float cameraPullBack = 2.5f;

	// ========== Filled while the game plays, not saved with the scene ==========

	bool         isRidden = false;
	unsigned int riderID  = 0;

	// What the rider had before it mounted, given back when it dismounts
	std::string savedIdleAnimation;
	std::string savedLockedOnIdleAnimation;
	bool        wasRiderKinematic = false;
	bool        wasRiderTrigger   = false;

	static void RegisterToInspector()
	{
		InspectorComponentRegistry::RegisterComponent<MountComponent>("Mount", EngineAssets::Icons::Skeleton,
			{
				ADD_PROPERTY(MountComponent, seatOffset, PropertyDataType::Float3),
				ADD_PROPERTY_RANGED(MountComponent, mountDistance, PropertyDataType::Float, 0.0f, MaxDistance),
				ADD_PROPERTY(MountComponent, mountPrompt,        PropertyDataType::String),
				ADD_PROPERTY(MountComponent, dismountPrompt,     PropertyDataType::String),
				ADD_PROPERTY(MountComponent, riderIdleAnimation, PropertyDataType::String),
				ADD_PROPERTY(MountComponent, riderWalkAnimation, PropertyDataType::String),
				ADD_PROPERTY(MountComponent, riderRunAnimation,  PropertyDataType::String),
				ADD_PROPERTY_RANGED(MountComponent, runAnimationFromSpeed, PropertyDataType::Float, 0.0f, 20.0f),
				ADD_PROPERTY_RANGED(MountComponent, cameraRise,            PropertyDataType::Float, 0.0f, MaxRise),
				ADD_PROPERTY_RANGED(MountComponent, cameraPullBack,        PropertyDataType::Float, 0.0f, MaxDistance)
			});
	}
};

}
