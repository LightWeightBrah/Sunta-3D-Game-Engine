#include "Core/SuntaPreCompiled.h"
#include "StaminaSystem.h"

#include "GameplayMath.h"
#include "ECS/Component.h"
#include "ECS/EntityManager.h"

namespace Sunta
{

namespace
{

using namespace GameplayMath;

// A push on the stick (or keys) shorter than this is not running, even with the run key held
constexpr float SmallestMoveToRun = 0.1f;

float GetActionCost(const StaminaComponent& stamina, CharacterAction action)
{
	switch (action)
	{
	case CharacterAction::Roll:        return stamina.rollCost;
	case CharacterAction::Backstep:    return stamina.backstepCost;
	case CharacterAction::LightAttack: return stamina.lightAttackCost;
	case CharacterAction::HeavyAttack: return stamina.heavyAttackCost;
	default:                           return 0.0f;
	}
}

void Spend(StaminaComponent& stamina, float amount)
{
	stamina.currentStamina   = glm::max(stamina.currentStamina - amount, 0.0f);
	stamina.secondsSinceUsed = 0.0f;
}

bool IsRunning(const CharacterControllerComponent* controller)
{
	return controller && controller->wantsToRun && !controller->isDrivenByAction
		&& glm::length(KeepOnlyHorizontalPart(controller->moveDirection)) >= SmallestMoveToRun;
}

bool IsActing(const CharacterActionsComponent* actions)
{
	return actions && actions->IsBusy();
}

void Refill(StaminaComponent& stamina, float deltaTime)
{
	stamina.secondsSinceUsed += deltaTime;

	if (stamina.secondsSinceUsed >= stamina.regenerationDelaySeconds)
		stamina.currentStamina = glm::min(stamina.currentStamina + stamina.regenerationPerSecond * deltaTime, stamina.maxStamina);
}

}

void StaminaSystem::PrepareForPlay(EntityManager& entityManager)
{
	for (StaminaComponent& stamina : entityManager.GetAllComponents<StaminaComponent>())
	{
		stamina.currentStamina   = stamina.maxStamina;
		stamina.secondsSinceUsed = stamina.regenerationDelaySeconds;
	}
}

void StaminaSystem::Update(EntityManager& entityManager, float deltaTime)
{
	for (StaminaComponent& stamina : entityManager.GetAllComponents<StaminaComponent>())
	{
		const unsigned int entityID = entityManager.GetEntityIDForComponent(stamina);

		if (IsRunning(entityManager.GetComponent<CharacterControllerComponent>(entityID)))
			Spend(stamina, stamina.runCostPerSecond * deltaTime);
		else if (IsActing(entityManager.GetComponent<CharacterActionsComponent>(entityID)))
			stamina.secondsSinceUsed = 0.0f;
		else
			Refill(stamina, deltaTime);
	}
}

bool StaminaSystem::TrySpendForAction(EntityManager& entityManager, unsigned int entityID, CharacterAction action)
{
	StaminaComponent* stamina = entityManager.GetComponent<StaminaComponent>(entityID);
	if (!stamina)
		return true;

	if (stamina->currentStamina <= 0.0f)
		return false;

	Spend(*stamina, GetActionCost(*stamina, action));
	return true;
}

bool StaminaSystem::HasStaminaLeft(EntityManager& entityManager, unsigned int entityID)
{
	const StaminaComponent* stamina = entityManager.GetComponent<StaminaComponent>(entityID);
	return !stamina || stamina->currentStamina > 0.0f;
}

}
