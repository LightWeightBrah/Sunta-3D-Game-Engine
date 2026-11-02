#include "Core/SuntaPreCompiled.h"
#include "HealingFlaskSystem.h"

#include <glm/glm.hpp>

#include "CharacterActionsSystem.h"
#include "Audio/Audio.h"
#include "Audio/AudioSystem.h"
#include "Core/InputManager.h"
#include "Core/KeyCodes.h"
#include "ECS/Component.h"
#include "ECS/EntityManager.h"
#include "Scene/EntityHierarchy.h"

namespace Sunta
{

namespace
{

constexpr int DrinkKey = SUNTA_KEY_R;

// How many specks the heal's effect throws at once
constexpr int HealEffectBurst = 60;

bool IsAlive(EntityManager& entityManager, unsigned int entityID)
{
	const HealthComponent* health = entityManager.GetComponent<HealthComponent>(entityID);
	return health && !health->isDead;
}

bool WantsToDrink(EntityManager& entityManager, unsigned int entityID)
{
	const CharacterControllerComponent* controller = entityManager.GetComponent<CharacterControllerComponent>(entityID);
	return controller && controller->isPlayerControlled && !controller->isHeldByCutscene && InputManager::IsKeyDown(DrinkKey);
}

void SetChildModelVisible(EntityManager& entityManager, unsigned int entityID, const std::string& childName, bool isVisible)
{
	if (childName.empty())
		return;

	for (unsigned int childID : EntityHierarchy::GetChildren(entityManager, entityID))
	{
		const TagComponent* tag   = entityManager.GetComponent<TagComponent>(childID);
		ModelComponent*     model = entityManager.GetComponent<ModelComponent>(childID);
		if (tag && model && tag->name == childName)
			model->isVisible = isVisible;
	}
}

// The hand that drinks lets go of what it holds (the shield) and takes the flask, or the other way round after the drink
void ShowDrinkingHand(EntityManager& entityManager, unsigned int entityID, const HealingFlaskComponent& flask, bool isDrinking)
{
	SetChildModelVisible(entityManager, entityID, flask.hiddenWhileDrinking, !isDrinking);
	SetChildModelVisible(entityManager, entityID, flask.shownWhileDrinking, isDrinking);
}

void BurstHealEffect(EntityManager& entityManager, unsigned int entityID, const std::string& effectName)
{
	if (effectName.empty())
		return;

	for (unsigned int childID : EntityHierarchy::GetChildren(entityManager, entityID))
	{
		const TagComponent*       tag     = entityManager.GetComponent<TagComponent>(childID);
		ParticleEmitterComponent* emitter = entityManager.GetComponent<ParticleEmitterComponent>(childID);
		if (tag && emitter && tag->name == effectName)
			emitter->burstCount += HealEffectBurst;
	}
}

// The health comes back and the drink is used up
void Heal(EntityManager& entityManager, unsigned int entityID, HealingFlaskComponent& flask)
{
	flask.hasHealed = true;
	flask.charges   = glm::max(flask.charges - 1, 0);

	if (HealthComponent* health = entityManager.GetComponent<HealthComponent>(entityID))
		health->currentHealth = glm::min(health->currentHealth + flask.healAmount, health->maxHealth);

	BurstHealEffect(entityManager, entityID, flask.healEffect);
	AudioSystem::PlayAtEntity(entityManager, entityID, flask.healSound);
}

// The heal comes at its moment of the drink; a drink stopped earlier (a hit) costs nothing
void UpdateDrink(EntityManager& entityManager, unsigned int entityID, HealingFlaskComponent& flask)
{
	const CharacterActionsComponent* actions = entityManager.GetComponent<CharacterActionsComponent>(entityID);
	const bool isStillDrinking = actions && actions->IsUsingItem();

	if (isStillDrinking && !flask.hasHealed && actions->actionProgress >= flask.healAtProgress)
		Heal(entityManager, entityID, flask);

	if (!isStillDrinking)
	{
		flask.isDrinking = false;
		ShowDrinkingHand(entityManager, entityID, flask, false);
	}
}

}

void HealingFlaskSystem::PrepareForPlay(EntityManager& entityManager)
{
	for (HealingFlaskComponent& flask : entityManager.GetAllComponents<HealingFlaskComponent>())
	{
		flask.charges    = flask.maxCharges;
		flask.isDrinking = false;
		flask.hasHealed  = false;
		ShowDrinkingHand(entityManager, entityManager.GetEntityIDForComponent(flask), flask, false);

		Audio::Preload(flask.drinkSound);
		Audio::Preload(flask.healSound);
	}
}

void HealingFlaskSystem::Update(EntityManager& entityManager)
{
	for (HealingFlaskComponent& flask : entityManager.GetAllComponents<HealingFlaskComponent>())
	{
		const unsigned int entityID = entityManager.GetEntityIDForComponent(flask);

		if (flask.isDrinking)
			UpdateDrink(entityManager, entityID, flask);
		else if (WantsToDrink(entityManager, entityID))
			Drink(entityManager, entityID);
	}
}

bool HealingFlaskSystem::Drink(EntityManager& entityManager, unsigned int entityID)
{
	HealingFlaskComponent* flask = entityManager.GetComponent<HealingFlaskComponent>(entityID);
	if (!flask || flask->isDrinking || flask->charges <= 0 || !IsAlive(entityManager, entityID))
		return false;

	const CharacterActionsComponent* actions = entityManager.GetComponent<CharacterActionsComponent>(entityID);
	const bool canAct = !actions || !actions->IsBusy() || actions->currentAction == CharacterAction::Block;
	if (!canAct)
		return false;

	AudioSystem::PlayAtEntity(entityManager, entityID, flask->drinkSound);

	// Without a drinking animation the flask heals at once
	if (!CharacterActionsSystem::UseItem(entityManager, entityID, flask->drinkAnimation))
	{
		Heal(entityManager, entityID, *flask);
		return true;
	}

	flask->isDrinking = true;
	flask->hasHealed  = false;
	ShowDrinkingHand(entityManager, entityID, *flask, true);
	return true;
}

void HealingFlaskSystem::Refill(EntityManager& entityManager, unsigned int entityID)
{
	if (HealingFlaskComponent* flask = entityManager.GetComponent<HealingFlaskComponent>(entityID))
		flask->charges = flask->maxCharges;
}

}
