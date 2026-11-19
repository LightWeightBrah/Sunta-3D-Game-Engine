#pragma once

#include <string>

#include "ECS/ComponentLayout.h"
#include "Core/EngineAssets.h"

namespace Sunta
{

// Aims a character's attacks at the enemy it fights: while it attacks, the spine bends up or down and turns left or
// right a little, so a punch made for an opponent of another height (or one standing a bit aside) meets the enemy's
// head or chest instead of its belly or the air. Common in action games ("aim offset")
//
// How to use it: Add Component > Strike Aim on the character (with Character Actions and an Animator). 'spineBones'
// are the bones that bend, from the hips up (e.g. "mixamorig:Spine,mixamorig:Spine1,mixamorig:Spine2"); the angle
// is shared between them. 'clipStrikeHeight' is where the character's attack animations strike by themselves (meters
// above its feet); the aim raises or lowers that to 'targetHeight' of the enemy's body (0 = feet, 1 = top of its
// body box: 0.85 is about the face). The enemy is the one locked on to, or the nearest character in front
struct StrikeAimComponent
{
	static constexpr float MaxAngleLimit = 60.0f;

	std::string spineBones;
	float clipStrikeHeight  = 1.25f;
	float targetHeight      = 0.85f;
	float maxPitchDegrees   = 20.0f;
	float maxYawDegrees     = 30.0f;
	float fadeSeconds       = 0.12f; // how quickly the aim comes in when an attack starts and goes when it ends
	float searchDistance    = 4.0f;  // the nearest character in front farther than this is not aimed at

	// ========== Filled while the game plays, not saved with the scene ==========
	float weight          = 0.0f;
	float pitchDegrees    = 0.0f;
	float yawDegrees      = 0.0f;
	bool  isApplied       = false;

	static void RegisterToInspector()
	{
		InspectorComponentRegistry::RegisterComponent<StrikeAimComponent>("Strike Aim", EngineAssets::Icons::CharacterTarget,
			{
				ADD_PROPERTY(StrikeAimComponent, spineBones, PropertyDataType::String),
				ADD_PROPERTY_RANGED(StrikeAimComponent, clipStrikeHeight, PropertyDataType::Float, 0.0f, 5.0f),
				ADD_PROPERTY_RANGED(StrikeAimComponent, targetHeight,     PropertyDataType::Float, 0.0f, 1.0f),
				ADD_PROPERTY_RANGED(StrikeAimComponent, maxPitchDegrees,  PropertyDataType::Float, 0.0f, MaxAngleLimit),
				ADD_PROPERTY_RANGED(StrikeAimComponent, maxYawDegrees,    PropertyDataType::Float, 0.0f, MaxAngleLimit),
				ADD_PROPERTY_RANGED(StrikeAimComponent, fadeSeconds,      PropertyDataType::Float, 0.0f, 1.0f),
				ADD_PROPERTY_RANGED(StrikeAimComponent, searchDistance,   PropertyDataType::Float, 0.5f, 20.0f)
			});
	}
};

}
