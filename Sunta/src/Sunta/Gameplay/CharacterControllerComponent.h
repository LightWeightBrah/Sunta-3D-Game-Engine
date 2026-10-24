#pragma once

#include <cstddef>
#include <glm/glm.hpp>

#include "ECS/ComponentLayout.h"
#include "Core/EngineAssets.h"

namespace Sunta
{

// Turns an entity into a character that walks and runs, no script needed
//
// How to use it: in the editor choose Add Component > Character Controller
// It also adds the Physics Body and the Box Collider a character needs
// After pressing Play the character moves with W A S D (relative to where the camera looks)
// and runs while Left Shift is held
//
// Every number below shows up in the Inspector, so the feel of the character is tuned there
//
// A script (or an enemy) can drive the character instead of the keyboard:
// turn 'isPlayerControlled' off, then write 'moveDirection' and 'wantsToRun' every frame
struct CharacterControllerComponent
{
	// Limits of the sliders in the Inspector
	static constexpr float MaxSpeed        = 20.0f;
	static constexpr float MaxAcceleration = 100.0f;
	static constexpr float MaxTurnSpeed    = 2000.0f;
	static constexpr float MaxFacingOffset = 180.0f;

	// Speeds are in meters per second
	float walkSpeed = 2.0f;
	float runSpeed  = 5.0f;

	// How quickly the character reaches its full speed and how quickly it stops
	// A high value feels light and snappy, a low value feels heavy
	float acceleration = 30.0f;

	// How quickly the character turns to face the direction it moves in (degrees per second)
	float turnSpeed = 720.0f;

	// Not every 3D model looks along the -Z axis, which the engine treats as "forward"
	// If the model walks sideways or backwards, set this to the angle that turns it the right way (e.g. 180)
	float facingOffsetDegrees = 0.0f;

	// On: reads the keyboard. Off: a script decides where the character goes
	bool isPlayerControlled = true;

	// Where the character wants to go, in the world (the height is ignored)
	// The length is how much of the full speed is used: 0 = stand still, 1 = full speed
	// Filled from the keyboard when 'isPlayerControlled' is on. Not saved with the scene
	glm::vec3 moveDirection = glm::vec3(0.0f);

	// Run instead of walk. Filled from the keyboard when 'isPlayerControlled' is on. Not saved with the scene
	bool wantsToRun = false;

	static void RegisterToInspector()
	{
		InspectorComponentRegistry::RegisterComponent<CharacterControllerComponent>("Character Controller", EngineAssets::Icons::Skeleton,
			{
				ADD_PROPERTY_RANGED(CharacterControllerComponent, walkSpeed,           PropertyDataType::Float, 0.0f, MaxSpeed),
				ADD_PROPERTY_RANGED(CharacterControllerComponent, runSpeed,            PropertyDataType::Float, 0.0f, MaxSpeed),
				ADD_PROPERTY_RANGED(CharacterControllerComponent, acceleration,        PropertyDataType::Float, 0.0f, MaxAcceleration),
				ADD_PROPERTY_RANGED(CharacterControllerComponent, turnSpeed,           PropertyDataType::Float, 0.0f, MaxTurnSpeed),
				ADD_PROPERTY_RANGED(CharacterControllerComponent, facingOffsetDegrees, PropertyDataType::Float, -MaxFacingOffset, MaxFacingOffset),
				ADD_PROPERTY(CharacterControllerComponent, isPlayerControlled, PropertyDataType::Bool)
			});
	}
};

}
