#pragma once

#include <cstddef>

#include "ECS/ComponentLayout.h"
#include "Core/EngineAssets.h"

namespace Sunta
{

// Lets a character lock on to an enemy (an entity with a Lock On Target component)
//
// How to use it: in the editor choose Add Component > Lock On on the player (next to the Character Controller)
// After pressing Play, Q locks on to the enemy the camera looks at, and Q again lets go
// While locked on, the mouse switches to another enemy (flick it left or right) instead of turning the camera,
// the camera frames the enemy (needs a Follow Camera) and the character faces it while walking (strafing)
// Running (Left Shift) turns the character towards the way it runs, as it does when it is not locked on
//
// A script (or an enemy) can use it too: turn 'isPlayerControlled' off, then call setTarget / clear from Lua
struct LockOnComponent
{
	// Limits of the sliders in the Inspector
	static constexpr float MaxDistanceLimit   = 100.0f;
	static constexpr float MaxSearchAngle     = 180.0f;
	static constexpr float MaxEyeHeight       = 5.0f;
	static constexpr float MaxSwitchDistance  = 1000.0f;

	// Enemies farther than this (in meters) can't be locked on to, and the lock is dropped when the enemy gets a bit farther
	float maxDistance = 15.0f;

	// How far from the middle of the screen an enemy can be and still be found by Q (degrees to the left and to the right)
	float searchAngleDegrees = 60.0f;

	// Where the character looks from, as a height above its position (used to see whether a wall is in the way)
	float eyeHeight = 1.5f;

	// How far the mouse has to be moved (in pixels, quickly) to switch to the next enemy
	float switchMouseDistance = 120.0f;

	// On: an enemy behind a wall can't be locked on to
	bool requireLineOfSight = true;

	// On: reads the keyboard and the mouse. Off: a script decides what is locked on to
	bool isPlayerControlled = true;

	// The enemy that is locked on to now. Not saved with the scene
	bool         hasTarget      = false;
	unsigned int targetEntityID = 0;

	// Working values of the mouse switching. Not saved with the scene
	float mouseTravel        = 0.0f;
	float switchCooldownLeft = 0.0f;

	static void RegisterToInspector()
	{
		InspectorComponentRegistry::RegisterComponent<LockOnComponent>("Lock On", EngineAssets::Icons::CharacterTarget,
			{
				ADD_PROPERTY_RANGED(LockOnComponent, maxDistance,         PropertyDataType::Float, 1.0f,  MaxDistanceLimit),
				ADD_PROPERTY_RANGED(LockOnComponent, searchAngleDegrees,  PropertyDataType::Float, 5.0f,  MaxSearchAngle),
				ADD_PROPERTY_RANGED(LockOnComponent, eyeHeight,           PropertyDataType::Float, 0.0f,  MaxEyeHeight),
				ADD_PROPERTY_RANGED(LockOnComponent, switchMouseDistance, PropertyDataType::Float, 20.0f, MaxSwitchDistance),
				ADD_PROPERTY(LockOnComponent, requireLineOfSight, PropertyDataType::Bool),
				ADD_PROPERTY(LockOnComponent, isPlayerControlled, PropertyDataType::Bool)
			});
	}
};

}
