#pragma once

#include <string>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include "ECS/ComponentLayout.h"
#include "Core/EngineAssets.h"

namespace Sunta
{

// A short scene played by the engine itself (the camera flies, characters move), e.g. a boss rising before the fight
//
// How to use it: Add Component > Cutscene on an empty entity, then give it children: the points the camera flies
// through, in order (their positions; the camera passes them smoothly in 'durationSeconds'). The camera looks at the entity
// named 'lookAtEntity' ('lookAtHeight' above its feet), or along its path when it is empty
// At 'animationAtSeconds' the entity named 'animationEntity' plays its animation 'animationName' once (a roar)
// During the last 'titleSeconds' the 'title' fades in over the picture (the boss's name). Black bars frame the picture
// While it plays nobody fights: the enemies wait and the player's keys do nothing; E skips it ('canBeSkipped')
// It starts from a Fog Gate (its 'cutscene': played the first time the fog closes behind the player) or from a script:
// this.cutscene:play()
// With 'videoFile' (e.g. @game/Videos/intro.mpg) a video plays instead of the engine's camera: a cutscene made elsewhere,
// e.g. by an AI video generator. It must be MPEG-1 (see VideoPlayer.h for the ffmpeg command), it plays without sound
// and ends with the video (or E). The camera points, the animation and the title are not used then. Its 'sound' (the
// video's own sound track as a separate file, e.g. @game/Sounds/Film/intro.ogg) starts with it and fades out when it ends
// With 'waitForKey' (a letter or a digit, e.g. "3") it is a moment the player answers: the camera reaches the end of
// its path and stays there, 'waitPrompt' is shown, and the cutscene ends only when that key is pressed (a script then
// goes on: this.cutscene.wasAnswered). With 'waitPresses' above 1 the key must be pressed that many times, quickly
// enough: what was pressed drains away at one press per 'pressWindowSeconds' (a slower tempo never gets there), and a
// bar under the prompt shows how far the player got (getting up off the ground, breaking free)
// 'blendInSeconds' above 0: the camera doesn't cut to the first point, it flies there from wherever it was (the shot
// before it, or the game's camera) over that long - shots of one sequence flow into each other
// 'fadeSeconds' above 0: no sudden cut - the game fades to black, the cutscene comes out of the black, and when it
// ends the game comes back out of the black (each fade that long)
struct CutsceneComponent
{
	// Limits of the sliders in the Inspector
	static constexpr float MaxSeconds = 120.0f;
	static constexpr float MaxHeight  = 20.0f;

	float       durationSeconds = 8.0f;
	std::string lookAtEntity;
	float       lookAtHeight    = 1.5f;
	// A two-shot: with a second entity named here the camera looks at the middle between the two (a face-off seen
	// from aside keeps both in the picture); empty = only 'lookAtEntity'
	std::string lookAlsoAtEntity;

	std::string animationEntity;
	std::string animationName;
	float       animationAtSeconds = 1.0f;

	std::string title;
	float       titleSeconds = 3.0f;

	bool        canBeSkipped = true;
	std::string videoFile;
	std::string sound;

	std::string waitForKey;
	std::string waitPrompt;
	int         waitPresses        = 1;
	float       pressWindowSeconds = 0.6f;
	// Played on every press of a many-press moment (a blow landing, a grunt): each press is felt, as in an action
	// game's quick-time event
	std::string pressSound;

	float       blendInSeconds = 0.0f;

	float       fadeSeconds = 0.0f;

	// ========== Filled while the game plays, not saved with the scene ==========

	bool  isPlaying           = false;
	// Seconds of the fade to black still left before it starts (see fadeSeconds)
	float fadeToBlackLeft     = 0.0f;
	bool  hasStartedSound     = false;
	unsigned int soundVoice   = 0;
	bool  hasPlayed           = false;
	bool  hasStartedAnimation = false;
	bool  isVideo             = false;
	float seconds             = 0.0f;
	// How far the presses got (see waitPresses), draining while the player doesn't press
	float pressProgress       = 0.0f;
	// 1 the moment the key is pressed, falling back to 0 (the button on the screen pops with each press)
	float pressPop            = 0.0f;
	// The player pressed 'waitForKey' (the last time it played)
	bool  wasAnswered         = false;
	// Where the camera was when it started (see blendInSeconds)
	glm::vec3 blendFromPosition = glm::vec3(0.0f);
	glm::quat blendFromRotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
	bool      canBlend          = false;
	// How far from what it looks at the camera may be without scenery in between (meters, < 0 = not measured yet)
	float clearDistance       = -1.0f;

	static void RegisterToInspector()
	{
		InspectorComponentRegistry::RegisterComponent<CutsceneComponent>("Cutscene", EngineAssets::Icons::StateMachine,
			{
				ADD_PROPERTY_RANGED(CutsceneComponent, durationSeconds, PropertyDataType::Float, 0.5f, MaxSeconds),
				ADD_PROPERTY(CutsceneComponent, lookAtEntity, PropertyDataType::String),
				ADD_PROPERTY_RANGED(CutsceneComponent, lookAtHeight, PropertyDataType::Float, 0.0f, MaxHeight),
				ADD_PROPERTY(CutsceneComponent, lookAlsoAtEntity, PropertyDataType::String),
				ADD_PROPERTY(CutsceneComponent, animationEntity, PropertyDataType::String),
				ADD_PROPERTY(CutsceneComponent, animationName, PropertyDataType::String),
				ADD_PROPERTY_RANGED(CutsceneComponent, animationAtSeconds, PropertyDataType::Float, 0.0f, MaxSeconds),
				ADD_PROPERTY(CutsceneComponent, title, PropertyDataType::String),
				ADD_PROPERTY_RANGED(CutsceneComponent, titleSeconds, PropertyDataType::Float, 0.0f, MaxSeconds),
				ADD_PROPERTY(CutsceneComponent, canBeSkipped, PropertyDataType::Bool),
				ADD_PROPERTY(CutsceneComponent, videoFile, PropertyDataType::String),
				ADD_PROPERTY(CutsceneComponent, sound, PropertyDataType::String),
				ADD_PROPERTY(CutsceneComponent, waitForKey, PropertyDataType::String),
				ADD_PROPERTY(CutsceneComponent, waitPrompt, PropertyDataType::String),
				ADD_PROPERTY(CutsceneComponent, pressSound, PropertyDataType::String),
				ADD_PROPERTY_RANGED(CutsceneComponent, waitPresses, PropertyDataType::Int, 1, 30),
				ADD_PROPERTY_RANGED(CutsceneComponent, pressWindowSeconds, PropertyDataType::Float, 0.1f, 3.0f),
				ADD_PROPERTY_RANGED(CutsceneComponent, blendInSeconds, PropertyDataType::Float, 0.0f, 10.0f),
				ADD_PROPERTY_RANGED(CutsceneComponent, fadeSeconds, PropertyDataType::Float, 0.0f, 5.0f)
			});
	}
};

}
