#pragma once

#include <cstddef>
#include <glm/glm.hpp>

#include "ECS/ComponentLayout.h"
#include "Core/EngineAssets.h"

namespace Sunta
{

// A camera that circles around the player and follows it, no script needed
//
// How to use it: in the editor choose Add Component > Follow Camera on the entity that has the Camera component
// (it adds the Camera component when it is missing). The player is the entity with a Character Controller that is player controlled
// After pressing Play press TAB to give the mouse to the game: the mouse turns the camera around the player
// Walls between the player and the camera push the camera closer, so the player is never hidden behind a wall
//
// With a Lock On on the player the camera turns on its own to the locked enemy, so the player and the enemy are both in view
// The Transform of the camera entity is the starting view: the camera begins looking the way the entity looks
//
// Every number below shows up in the Inspector, so the feel of the camera is tuned there
struct FollowCameraComponent
{
	// Limits of the sliders in the Inspector
	static constexpr float MaxDistance        = 20.0f;
	static constexpr float MaxPivotHeight     = 5.0f;
	static constexpr float MaxSensitivity     = 1.0f;
	static constexpr float MaxPitchLimit      = 85.0f;
	static constexpr float MaxSharpness       = 50.0f;
	static constexpr float MaxShoulderOffset  = 2.0f;
	static constexpr float MaxViewOffset      = 45.0f;

	// How far behind the player the camera is (meters)
	float distance = 4.0f;

	// The camera circles around a point above the position of the player: the height of that point (the shoulders, the head)
	float pivotHeight = 1.6f;

	// Moves the camera to the side of the player (meters, positive = to the right, negative = to the left)
	// A small number gives the "over the shoulder" view, where the player is not hiding the middle of the screen
	float shoulderOffset = 0.0f;

	// Turns the view of the camera without moving the camera, which moves the player on the screen (degrees)
	// Yaw: a positive number turns the view to the left, so the player moves to the right of the screen
	// Pitch: a positive number looks further down, so the player moves up on the screen
	float viewYawOffsetDegrees   = 0.0f;
	float viewPitchOffsetDegrees = 0.0f;

	// Degrees the camera turns for every pixel the mouse moves
	float mouseSensitivity = 0.15f;

	// On: moving the mouse up looks down
	bool invertY = false;

	// How far down and up the camera can look (degrees). A positive number looks down at the player, a negative one looks up
	float minPitchDegrees = -15.0f;
	float maxPitchDegrees = 60.0f;

	// How quickly the camera catches up with the player (higher = tighter, lower = floatier)
	float followSharpness = 15.0f;

	// While locked on: how far down the camera looks (degrees) and how quickly it turns to the enemy
	float lockOnPitchDegrees = 15.0f;
	float lockOnSharpness    = 6.0f;

	// Working values of the camera. Not saved with the scene
	bool      isInitialized   = false;
	float     yawDegrees      = 0.0f;
	float     pitchDegrees    = 0.0f;
	float     currentDistance = 0.0f; // 'distance', or less while a wall pushes the camera
	glm::vec3 pivot           = glm::vec3(0.0f);

	static void RegisterToInspector()
	{
		InspectorComponentRegistry::RegisterComponent<FollowCameraComponent>("Follow Camera", EngineAssets::Icons::Transform,
			{
				ADD_PROPERTY_RANGED(FollowCameraComponent, distance,           PropertyDataType::Float, 0.5f,  MaxDistance),
				ADD_PROPERTY_RANGED(FollowCameraComponent, pivotHeight,        PropertyDataType::Float, 0.0f,  MaxPivotHeight),
				ADD_PROPERTY_RANGED(FollowCameraComponent, shoulderOffset,     PropertyDataType::Float, -MaxShoulderOffset, MaxShoulderOffset),
				ADD_PROPERTY_RANGED(FollowCameraComponent, viewYawOffsetDegrees,   PropertyDataType::Float, -MaxViewOffset, MaxViewOffset),
				ADD_PROPERTY_RANGED(FollowCameraComponent, viewPitchOffsetDegrees, PropertyDataType::Float, -MaxViewOffset, MaxViewOffset),
				ADD_PROPERTY_RANGED(FollowCameraComponent, mouseSensitivity,   PropertyDataType::Float, 0.01f, MaxSensitivity),
				ADD_PROPERTY(FollowCameraComponent, invertY, PropertyDataType::Bool),
				ADD_PROPERTY_RANGED(FollowCameraComponent, minPitchDegrees,    PropertyDataType::Float, -MaxPitchLimit, MaxPitchLimit),
				ADD_PROPERTY_RANGED(FollowCameraComponent, maxPitchDegrees,    PropertyDataType::Float, -MaxPitchLimit, MaxPitchLimit),
				ADD_PROPERTY_RANGED(FollowCameraComponent, followSharpness,    PropertyDataType::Float, 1.0f,  MaxSharpness),
				ADD_PROPERTY_RANGED(FollowCameraComponent, lockOnPitchDegrees, PropertyDataType::Float, -MaxPitchLimit, MaxPitchLimit),
				ADD_PROPERTY_RANGED(FollowCameraComponent, lockOnSharpness,    PropertyDataType::Float, 1.0f,  MaxSharpness)
			});
	}
};

}
