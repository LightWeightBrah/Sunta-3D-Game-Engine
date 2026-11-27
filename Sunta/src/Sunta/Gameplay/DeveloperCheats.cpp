#include "Core/SuntaPreCompiled.h"
#include "DeveloperCheats.h"

#include "HUDSystem.h"
#include "Core/InputManager.h"
#include "Core/KeyCodes.h"
#include "ECS/Component.h"
#include "ECS/EntityManager.h"
#include "Renderer/PostProcessing.h"

namespace Sunta
{

namespace
{

constexpr int   ImmortalKey       = SUNTA_KEY_F1;
constexpr float ImmortalFloorPart = 0.1f;

// F3 shows the scene as pixel art made of 3D (see EnvironmentComponent: big pixels, few colors, inked edges) and back,
// to compare the looks while testing
constexpr int PixelLookKey = SUNTA_KEY_F3;
// the first pixel look against the new one (see PostProcessing::isClassicPixelLook)
constexpr int ClassicPixelKey = SUNTA_KEY_F4;

struct SavedLook
{
	bool  isPixel        = false;
	int   pixelSize      = 1;
	int   paletteLevels  = 0;
	float ditherStrength = 0.5f;
	float outline        = 0.0f;
	int   paintRadius    = 0;
};

SavedLook& GetSavedLook()
{
	static SavedLook saved;
	return saved;
}

void TogglePixelLook(EntityManager& entityManager)
{
	auto& environments = entityManager.GetAllComponents<EnvironmentComponent>();
	if (environments.empty())
		return;

	EnvironmentComponent& environment = environments.front();
	SavedLook& saved = GetSavedLook();
	if (!saved.isPixel)
	{
		saved = { true, environment.pixelSize, environment.paletteLevels, environment.ditherStrength,
		          environment.outlineStrength, environment.paintRadius };
		// 16 levels a channel and no dither: clean flat areas of color (a dither pattern read as a checkered noise
		// and, fixed to the screen, crawled over the world whenever the camera moved; 7 levels without it banded)
		environment.pixelSize       = 4;
		environment.paletteLevels   = 16;
		environment.ditherStrength  = 0.0f;
		environment.outlineStrength = 0.7f;
		environment.paintRadius     = 0;
	}
	else
	{
		environment.pixelSize       = saved.pixelSize;
		environment.paletteLevels   = saved.paletteLevels;
		environment.ditherStrength  = saved.ditherStrength;
		environment.outlineStrength = saved.outline;
		environment.paintRadius     = saved.paintRadius;
		saved.isPixel = false;
	}
	HUDSystem::ShowNotification(entityManager, saved.isPixel ? "PIXEL LOOK ON (F3)" : "PIXEL LOOK OFF (F3)");
}

}

void DeveloperCheats::Update(EntityManager& entityManager)
{
	if (!isAllowed)
		return;

	if (InputManager::IsKeyDown(ImmortalKey))
	{
		isPlayerImmortal = !isPlayerImmortal;
		HUDSystem::ShowNotification(entityManager, isPlayerImmortal ? "IMMORTAL MODE ON (F1)" : "IMMORTAL MODE OFF (F1)");
	}

	if (InputManager::IsKeyDown(PixelLookKey))
		TogglePixelLook(entityManager);

	if (InputManager::IsKeyDown(ClassicPixelKey))
	{
		PostProcessing::isClassicPixelLook = !PostProcessing::isClassicPixelLook;
		HUDSystem::ShowNotification(entityManager, PostProcessing::isClassicPixelLook ? "OLD PIXEL LOOK (F4)" : "NEW PIXEL LOOK (F4)");
	}

	if (!isPlayerImmortal)
		return;

	for (CharacterControllerComponent& controller : entityManager.GetAllComponents<CharacterControllerComponent>())
	{
		if (!controller.isPlayerControlled)
			continue;

		const unsigned int playerID = entityManager.GetEntityIDForComponent(controller);
		if (StaminaComponent* stamina = entityManager.GetComponent<StaminaComponent>(playerID))
			stamina->currentStamina = stamina->maxStamina;
	}
}

float DeveloperCheats::GetPlayerHealthFloor(float maxHealth)
{
	return isAllowed && isPlayerImmortal ? maxHealth * ImmortalFloorPart : 0.0f;
}

}
