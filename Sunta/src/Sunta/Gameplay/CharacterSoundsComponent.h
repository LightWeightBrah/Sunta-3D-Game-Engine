#pragma once

#include <string>
#include <glm/glm.hpp>

#include "ECS/ComponentLayout.h"
#include "Core/EngineAssets.h"

namespace Sunta
{

// The sounds a character makes in a fight, played by the engine at the right moment, heard from where the character is
//
// How to use it: Add Component > Character Sounds on a character (the player, an enemy, a boss). Each field is a sound
// file or a FOLDER of them (one at random each time, e.g. @game/Sounds/Footsteps/Stone); an empty field is silent
//   attackSound    the weapon cuts the air: starts 'attackSoundLeadSeconds' before the blow lands (the start of the
//                  attack's hit window), so the loudest part of the swoosh meets the blade
//   hurtSound      a weapon or a shockwave hits the body
//   blockSound     the shield takes a blow (also when the guard breaks)
//   parrySound     the shield turns an attack away
//   deathSound     the character falls
//   rollSound      a roll or a backstep
//   footstepSound  each time a foot of the animation touches the ground: the bones 'leftFootBone' and 'rightFootBone'
//                  (a model without them: every 'footstepMeters' walked)
//   shockwaveSound a heavy blow shakes the ground (Character Actions with a shockwave)
// 'volume' is for all of them (a giant is louder than a ghoul)
struct CharacterSoundsComponent
{
	// Limits of the sliders in the Inspector
	static constexpr float MaxVolume         = 3.0f;
	static constexpr float MaxFootstepMeters = 5.0f;
	static constexpr float MaxLeadSeconds    = 1.0f;
	static constexpr int   FootCount         = 2;

	std::string attackSound;
	std::string hurtSound;
	std::string blockSound;
	std::string parrySound;
	std::string deathSound;
	std::string rollSound;
	std::string footstepSound;
	std::string shockwaveSound;

	float       attackSoundLeadSeconds = 0.15f;
	std::string leftFootBone           = "Foot_L";
	std::string rightFootBone          = "Foot_R";
	float       footstepMeters         = 1.4f;
	float       volume                 = 1.0f;

	// ========== Filled while the game plays, not saved with the scene ==========

	float     metersSinceFootstep = 0.0f;
	glm::vec3 lastPosition        = glm::vec3(0.0f);
	bool      hasLastPosition     = false;

	unsigned int soundedAttackNumber = 0;

	// Each foot (left, right): the lowest its bone gets above the character's feet, and whether it is in the air
	float lowestFootHeight[FootCount]   = { 0.0f, 0.0f };
	float previousFootHeight[FootCount] = { 0.0f, 0.0f };
	bool  isFootLifted[FootCount]       = { false, false };
	bool  isFootGoingDown[FootCount]    = { false, false };
	bool  hasFootHeights              = false;

	static void RegisterToInspector()
	{
		InspectorComponentRegistry::RegisterComponent<CharacterSoundsComponent>("Character Sounds", EngineAssets::Icons::AudioFile,
			{
				ADD_PROPERTY(CharacterSoundsComponent, attackSound, PropertyDataType::String),
				ADD_PROPERTY(CharacterSoundsComponent, hurtSound, PropertyDataType::String),
				ADD_PROPERTY(CharacterSoundsComponent, blockSound, PropertyDataType::String),
				ADD_PROPERTY(CharacterSoundsComponent, parrySound, PropertyDataType::String),
				ADD_PROPERTY(CharacterSoundsComponent, deathSound, PropertyDataType::String),
				ADD_PROPERTY(CharacterSoundsComponent, rollSound, PropertyDataType::String),
				ADD_PROPERTY(CharacterSoundsComponent, footstepSound, PropertyDataType::String),
				ADD_PROPERTY(CharacterSoundsComponent, shockwaveSound, PropertyDataType::String),
				ADD_PROPERTY_RANGED(CharacterSoundsComponent, attackSoundLeadSeconds, PropertyDataType::Float, 0.0f, MaxLeadSeconds),
				ADD_PROPERTY(CharacterSoundsComponent, leftFootBone, PropertyDataType::String),
				ADD_PROPERTY(CharacterSoundsComponent, rightFootBone, PropertyDataType::String),
				ADD_PROPERTY_RANGED(CharacterSoundsComponent, footstepMeters, PropertyDataType::Float, 0.2f, MaxFootstepMeters),
				ADD_PROPERTY_RANGED(CharacterSoundsComponent, volume, PropertyDataType::Float, 0.0f, MaxVolume)
			});
	}
};

}
