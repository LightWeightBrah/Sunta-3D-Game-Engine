#pragma once

#include <string>

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
// and ends with the video (or E). The camera points, the animation and the title are not used then
struct CutsceneComponent
{
	// Limits of the sliders in the Inspector
	static constexpr float MaxSeconds = 120.0f;
	static constexpr float MaxHeight  = 20.0f;

	float       durationSeconds = 8.0f;
	std::string lookAtEntity;
	float       lookAtHeight    = 1.5f;

	std::string animationEntity;
	std::string animationName;
	float       animationAtSeconds = 1.0f;

	std::string title;
	float       titleSeconds = 3.0f;

	bool        canBeSkipped = true;
	std::string videoFile;

	// ========== Filled while the game plays, not saved with the scene ==========

	bool  isPlaying           = false;
	bool  hasPlayed           = false;
	bool  hasStartedAnimation = false;
	bool  isVideo             = false;
	float seconds             = 0.0f;

	static void RegisterToInspector()
	{
		InspectorComponentRegistry::RegisterComponent<CutsceneComponent>("Cutscene", EngineAssets::Icons::StateMachine,
			{
				ADD_PROPERTY_RANGED(CutsceneComponent, durationSeconds, PropertyDataType::Float, 0.5f, MaxSeconds),
				ADD_PROPERTY(CutsceneComponent, lookAtEntity, PropertyDataType::String),
				ADD_PROPERTY_RANGED(CutsceneComponent, lookAtHeight, PropertyDataType::Float, 0.0f, MaxHeight),
				ADD_PROPERTY(CutsceneComponent, animationEntity, PropertyDataType::String),
				ADD_PROPERTY(CutsceneComponent, animationName, PropertyDataType::String),
				ADD_PROPERTY_RANGED(CutsceneComponent, animationAtSeconds, PropertyDataType::Float, 0.0f, MaxSeconds),
				ADD_PROPERTY(CutsceneComponent, title, PropertyDataType::String),
				ADD_PROPERTY_RANGED(CutsceneComponent, titleSeconds, PropertyDataType::Float, 0.0f, MaxSeconds),
				ADD_PROPERTY(CutsceneComponent, canBeSkipped, PropertyDataType::Bool),
				ADD_PROPERTY(CutsceneComponent, videoFile, PropertyDataType::String)
			});
	}
};

}
