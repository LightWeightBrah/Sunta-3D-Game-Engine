#include "Core/SuntaPreCompiled.h"
#include "DeveloperCheats.h"

#include "HUDSystem.h"
#include "Core/InputManager.h"
#include "Core/KeyCodes.h"
#include "ECS/Component.h"
#include "ECS/EntityManager.h"

namespace Sunta
{

namespace
{

constexpr int   ImmortalKey       = SUNTA_KEY_F1;
constexpr float ImmortalFloorPart = 0.1f;

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
