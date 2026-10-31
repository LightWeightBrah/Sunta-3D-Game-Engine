#pragma once

#include <string>

#include "ECS/ComponentLayout.h"
#include "Core/EngineAssets.h"

namespace Sunta
{

// What the player sees on the screen about itself: the health and stamina bars in the top left corner,
// a dot on the locked-on enemy, and the death message when it dies (the Dark Souls style HUD)
//
// How to use it: Add Component > Player HUD on the player (the Souls Player has it). It reads the Health, Stamina and Lock On
// of the same entity. The bars grow with the maximum health and stamina, as in Dark Souls
struct PlayerHUDComponent
{
	// Limit of the sliders in the Inspector
	static constexpr float MaxPixelsPerPoint = 20.0f;

	// How long the bars are: pixels for one point of health / stamina
	float pixelsPerHealthPoint  = 3.0f;
	float pixelsPerStaminaPoint = 2.6f;

	bool showLockOnMarker = true;

	std::string deathMessage = "YOU DIED";

	// ========== Filled while the game plays, not saved with the scene ==========

	// The pale part of the health bar: the health just lost, shrinking slowly after the hit
	float trailingHealth = 0.0f;

	float secondsSinceDeath = 0.0f;

	// What the player can do right here (e.g. "Rest at the bonfire [E]"), shown at the bottom. Set every frame, cleared after drawing
	std::string promptText;

	// A big message in the middle that fades away (e.g. "BONFIRE LIT")
	std::string notificationText;
	float notificationSecondsLeft = 0.0f;

	static void RegisterToInspector()
	{
		InspectorComponentRegistry::RegisterComponent<PlayerHUDComponent>("Player HUD", EngineAssets::Icons::Bonfire,
			{
				ADD_PROPERTY_RANGED(PlayerHUDComponent, pixelsPerHealthPoint,  PropertyDataType::Float, 0.0f, MaxPixelsPerPoint),
				ADD_PROPERTY_RANGED(PlayerHUDComponent, pixelsPerStaminaPoint, PropertyDataType::Float, 0.0f, MaxPixelsPerPoint),
				ADD_PROPERTY(PlayerHUDComponent, showLockOnMarker, PropertyDataType::Bool),
				ADD_PROPERTY(PlayerHUDComponent, deathMessage,     PropertyDataType::String)
			});
	}
};

}
