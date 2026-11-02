#include "Core/SuntaPreCompiled.h"
#include "CurrencySystem.h"

#include <glm/glm.hpp>

#include "CharacterControllerSystem.h"
#include "EntityName.h"
#include "GameplayMath.h"
#include "HUDSystem.h"
#include "Audio/Audio.h"
#include "Audio/AudioSystem.h"
#include "Core/InputManager.h"
#include "Core/KeyCodes.h"
#include "Core/Log.h"
#include "ECS/Component.h"
#include "ECS/EntityManager.h"
#include "Scene/EntityHierarchy.h"

namespace Sunta
{

namespace
{

constexpr int         RetrieveKey       = SUNTA_KEY_E;
constexpr const char* RetrieveKeyLabel  = "   [E]";

// The HUD counts up to a new amount in about this time, and shows the gain above it for this long
constexpr float CountUpSeconds   = 0.8f;
constexpr float ShowGainSeconds  = 2.5f;

bool IsOnPlayersSide(EntityManager& entityManager, unsigned int entityID)
{
	const CharacterControllerComponent* controller = entityManager.GetComponent<CharacterControllerComponent>(entityID);
	return controller && controller->isPlayerControlled;
}

bool IsAlive(EntityManager& entityManager, unsigned int entityID)
{
	const HealthComponent* health = entityManager.GetComponent<HealthComponent>(entityID);
	return !health || !health->isDead;
}

// The one who gets a reward: the attacker when it collects Currency, otherwise the player (a kill by a trap or a script)
CurrencyComponent* FindCollector(EntityManager& entityManager, unsigned int attackerEntityID)
{
	if (CurrencyComponent* currency = entityManager.GetComponent<CurrencyComponent>(attackerEntityID))
		return currency;

	unsigned int playerID = 0;
	return CharacterControllerSystem::TryFindPlayer(entityManager, playerID) ? entityManager.GetComponent<CurrencyComponent>(playerID) : nullptr;
}

// ========== The spot where the player fell ==========

void SetEmittersOf(EntityManager& entityManager, unsigned int entityID, bool isEmitting)
{
	std::vector<unsigned int> entities = { entityID };
	for (unsigned int childID : EntityHierarchy::GetChildren(entityManager, entityID))
		entities.push_back(childID);

	for (unsigned int id : entities)
	{
		if (ParticleEmitterComponent* emitter = entityManager.GetComponent<ParticleEmitterComponent>(id))
			emitter->isEmitting = isEmitting;
	}
}

// The marker entity stands on the dropped Currency and glows while something lies there
void UpdateDropMarker(EntityManager& entityManager, const CurrencyComponent& currency)
{
	unsigned int markerID = 0;
	if (!TryFindEntityByName(entityManager, currency.dropMarker, markerID))
		return;

	if (currency.HasDrop())
	{
		if (TransformComponent* transform = entityManager.GetComponent<TransformComponent>(markerID))
		{
			transform->position = currency.droppedPosition;
			transform->isDirty  = true;
		}
	}

	SetEmittersOf(entityManager, markerID, currency.HasDrop());
}

void Drop(EntityManager& entityManager, unsigned int entityID, CurrencyComponent& currency)
{
	if (currency.HasDrop())
		SUNTA_ENGINE_LOG_INFO("Currency: '{0}' lost {1} {2} for good", GetEntityName(entityManager, entityID), currency.droppedAmount, currency.displayName);

	const TransformComponent* transform = entityManager.GetComponent<TransformComponent>(entityID);

	currency.droppedAmount   = currency.amount;
	currency.droppedPosition = transform ? transform->position : glm::vec3(0.0f);
	currency.amount          = 0;
	currency.shownAmount     = 0.0f;

	UpdateDropMarker(entityManager, currency);
}

void Retrieve(EntityManager& entityManager, unsigned int entityID, CurrencyComponent& currency)
{
	const int amount = currency.droppedAmount;
	currency.droppedAmount = 0;
	UpdateDropMarker(entityManager, currency);

	AudioSystem::PlayAtEntity(entityManager, entityID, currency.retrieveSound);
	CurrencySystem::Add(entityManager, entityID, amount);
}

// Standing on the spot: with E (and the prompt) or just by walking over it
void OfferRetrieve(EntityManager& entityManager, unsigned int entityID, CurrencyComponent& currency)
{
	const TransformComponent* transform = entityManager.GetComponent<TransformComponent>(entityID);
	if (!currency.HasDrop() || !transform || !IsAlive(entityManager, entityID))
		return;

	const float distance = glm::length(GameplayMath::KeepOnlyHorizontalPart(transform->position - currency.droppedPosition));
	if (distance > currency.retrieveDistance)
		return;

	if (!currency.isRetrievedWithKey)
	{
		Retrieve(entityManager, entityID, currency);
		return;
	}

	HUDSystem::ShowPrompt(entityManager, currency.retrievePrompt + RetrieveKeyLabel);
	if (InputManager::IsKeyDown(RetrieveKey))
		Retrieve(entityManager, entityID, currency);
}

// The number in the HUD runs up to the real amount instead of jumping
void UpdateShownAmount(CurrencyComponent& currency, float deltaTime)
{
	const float target = (float)currency.amount;
	const float step   = glm::max(glm::abs(target - currency.shownAmount), 1.0f) * deltaTime / CountUpSeconds;

	currency.shownAmount = currency.shownAmount < target ? glm::min(currency.shownAmount + step, target) : target;
	currency.lastGainSecondsLeft = glm::max(currency.lastGainSecondsLeft - deltaTime, 0.0f);
}

}

void CurrencySystem::PrepareForPlay(EntityManager& entityManager)
{
	for (CurrencyComponent& currency : entityManager.GetAllComponents<CurrencyComponent>())
	{
		currency.droppedAmount       = 0;
		currency.shownAmount         = (float)currency.amount;
		currency.lastGain            = 0;
		currency.lastGainSecondsLeft = 0.0f;

		UpdateDropMarker(entityManager, currency);
		Audio::Preload(currency.gainSound);
		Audio::Preload(currency.retrieveSound);
	}
}

void CurrencySystem::Update(EntityManager& entityManager, float deltaTime)
{
	for (CurrencyComponent& currency : entityManager.GetAllComponents<CurrencyComponent>())
	{
		OfferRetrieve(entityManager, entityManager.GetEntityIDForComponent(currency), currency);
		UpdateShownAmount(currency, deltaTime);
	}
}

void CurrencySystem::OnDeath(EntityManager& entityManager, unsigned int deadEntityID, unsigned int attackerEntityID)
{
	const CurrencyRewardComponent* reward = entityManager.GetComponent<CurrencyRewardComponent>(deadEntityID);
	if (reward && reward->amount > 0 && !IsOnPlayersSide(entityManager, deadEntityID))
	{
		if (CurrencyComponent* collector = FindCollector(entityManager, attackerEntityID))
			Add(entityManager, entityManager.GetEntityIDForComponent(*collector), reward->amount);
	}

	CurrencyComponent* currency = entityManager.GetComponent<CurrencyComponent>(deadEntityID);
	if (currency && currency->isDroppedOnDeath)
		Drop(entityManager, deadEntityID, *currency);
}

void CurrencySystem::Add(EntityManager& entityManager, unsigned int entityID, int amount)
{
	CurrencyComponent* currency = entityManager.GetComponent<CurrencyComponent>(entityID);
	if (!currency || amount == 0)
		return;

	currency->amount = glm::max(currency->amount + amount, 0);

	if (amount > 0)
	{
		// Gains close together add up in the HUD (two enemies felled at once)
		currency->lastGain            = currency->lastGainSecondsLeft > 0.0f ? currency->lastGain + amount : amount;
		currency->lastGainSecondsLeft = ShowGainSeconds;
		AudioSystem::PlayAtEntity(entityManager, entityID, currency->gainSound);
	}
	else
		currency->shownAmount = (float)currency->amount;
}

}
