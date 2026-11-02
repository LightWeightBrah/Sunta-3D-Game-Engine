#pragma once

#include <cstddef>
#include <string>

#include "ECS/ComponentLayout.h"
#include "Core/EngineAssets.h"

namespace Sunta
{

// Plays the right animation for what the character is doing, no script needed: standing, walking or running,
// and while locked on to an enemy (facing it): stepping sideways and backwards
//
// How to use it: in the editor choose Add Component > Character Animation on the character
// (the entity needs a Model with animations, it gets the Animator automatically)
// The animations are chosen from the dropdowns in the Inspector, they list the animations of the Model
// Animations called Idle, Walk, Run, Walk_Back, Strafe_Left and Strafe_Right are chosen for you when the component is added
//
// Which one plays depends on how fast the character really moves (the Physics Body), so a character that
// walks into a wall stands still. Running is chosen above the middle between the walk speed and the run speed
// of the Character Controller. An animation that is not set is replaced by the closest one (running by walking, walking by standing,
// stepping sideways or backwards by walking)
struct CharacterAnimationComponent
{
	// Limit of the slider in the Inspector
	static constexpr float MaxBlendSeconds = 1.0f;

	// Names of the animations of the Model, empty means "not used"
	std::string idleAnimation;
	std::string walkAnimation;
	std::string runAnimation;

	// While locked on: the character faces the enemy and steps the way the player pushes
	std::string walkBackAnimation;
	std::string strafeLeftAnimation;
	std::string strafeRightAnimation;

	// How long one animation takes to change into the next one. 0 is a hard cut
	float blendSeconds = 0.2f;

	// How fast the character moves in the walking and the running animation (meters per second, at the model's size).
	// The animation then plays faster or slower with the real speed, so the feet stay planted on the ground
	// (slowing down, walking behind a shield). 0 = always at its own speed
	float walkAnimationSpeed = 0.0f;
	float runAnimationSpeed  = 0.0f;

	// The speed the walking animation plays at now (eased, the measured speed jitters). Not saved with the scene
	float currentPlaybackSpeed = 1.0f;

	static void RegisterToInspector()
	{
		// The Inspector draws it with a drawer (dropdowns of the animations), so there is no list of properties
		InspectorComponentRegistry::RegisterComponent<CharacterAnimationComponent>("Character Animation", EngineAssets::Icons::AnimationController, { });
	}
};

}
