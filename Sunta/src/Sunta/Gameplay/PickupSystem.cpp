#include "Core/SuntaPreCompiled.h"
#include "PickupSystem.h"

#include <algorithm>
#include <random>

#include "Audio/Audio.h"
#include "BonfireSystem.h"
#include "CharacterControllerSystem.h"
#include "EntityName.h"
#include "GameplayMath.h"
#include "HUDSystem.h"
#include "MountSystem.h"
#include "PickupComponent.h"
#include "Core/InputManager.h"
#include "Core/KeyCodes.h"
#include "Core/Log.h"
#include "ECS/Component.h"
#include "ECS/EntityManager.h"
#include "Scene/EntityHierarchy.h"
#include "Scene/WorldSpawner.h"

namespace Sunta
{

namespace
{

using namespace GameplayMath;

constexpr int PickupKey = SUNTA_KEY_E;
// A read text stays this long per letter when the book has no voice and no time of its own (and at least this long)
constexpr float SecondsPerLetter = 0.06f;
constexpr float ShortestReading  = 4.0f;
// A dropped item rises a little off the ground where its owner fell
constexpr float DropHeight = 0.1f;

std::mt19937& GetRandom()
{
	static std::mt19937 random(std::random_device{}());
	return random;
}

void Hide(EntityManager& entityManager, unsigned int entityID)
{
	if (ModelComponent* model = entityManager.GetComponent<ModelComponent>(entityID))
		model->isVisible = false;
	if (ParticleEmitterComponent* emitter = entityManager.GetComponent<ParticleEmitterComponent>(entityID))
		emitter->isEmitting = false;
	for (unsigned int childID : EntityHierarchy::GetChildren(entityManager, entityID))
		Hide(entityManager, childID);
}

void Take(EntityManager& entityManager, PickupComponent& pickup, unsigned int pickupID, unsigned int playerID)
{
	pickup.isTaken = true;
	Hide(entityManager, pickupID);
	if (!pickup.itemId.empty())
		PickupSystem::AddItem(entityManager, playerID, pickup.itemId);

	const std::string name = pickup.displayName.empty() ? pickup.itemId : pickup.displayName;
	HUDSystem::ShowNotification(entityManager, pickup.obtainedText + name);
	if (!pickup.pickupSound.empty())
		Audio::Play(pickup.pickupSound);

	if (!pickup.readText.empty())
	{
		float seconds = pickup.readSeconds;
		if (!pickup.voiceSound.empty())
		{
			AudioPlayOptions options;
			options.ducksMusic = true;
			Audio::Play(pickup.voiceSound, options);
		}
		if (seconds <= 0.0f)
			seconds = std::max(ShortestReading, SecondsPerLetter * static_cast<float>(pickup.readText.size()));
		HUDSystem::ShowReading(entityManager, name, pickup.readText, seconds);
	}

	SUNTA_ENGINE_LOG_INFO("Pickup: '{0}' took '{1}'", GetEntityName(entityManager, playerID), name);
}

void DropLoot(EntityManager& entityManager)
{
	for (LootDropComponent& loot : entityManager.GetAllComponents<LootDropComponent>())
	{
		const unsigned int ownerID = entityManager.GetEntityIDForComponent(loot);
		const HealthComponent* health = entityManager.GetComponent<HealthComponent>(ownerID);
		if (!health)
			continue;
		if (!health->isDead)
		{
			loot.hasDroppedThisLife = false;
			continue;
		}
		if (loot.hasDroppedThisLife || (loot.onlyOnce && loot.hasEverDropped))
			continue;
		loot.hasDroppedThisLife = true;
		if (std::uniform_real_distribution<float>(0.0f, 1.0f)(GetRandom()) > loot.chance)
			continue;

		const TransformComponent* owner = entityManager.GetComponent<TransformComponent>(ownerID);
		unsigned int copyID = 0;
		if (owner && WorldSpawner::Spawn(entityManager, loot.pickupTemplate, owner->position + glm::vec3(0.0f, DropHeight, 0.0f), 0.0f, copyID))
		{
			loot.hasEverDropped = true;
			if (PickupComponent* pickup = entityManager.GetComponent<PickupComponent>(copyID))
				pickup->isTaken = false;
			SUNTA_ENGINE_LOG_INFO("Pickup: '{0}' dropped '{1}'", GetEntityName(entityManager, ownerID), loot.pickupTemplate);
		}
	}
}

}

void PickupSystem::PrepareForPlay(EntityManager& entityManager)
{
	for (PickupComponent& pickup : entityManager.GetAllComponents<PickupComponent>())
		pickup.isTaken = false;
	for (LootDropComponent& loot : entityManager.GetAllComponents<LootDropComponent>())
	{
		loot.hasDroppedThisLife = false;
		loot.hasEverDropped     = false;
	}
	for (InventoryComponent& inventory : entityManager.GetAllComponents<InventoryComponent>())
		inventory.items.clear();
}

void PickupSystem::Update(EntityManager& entityManager, float deltaTime)
{
	(void)deltaTime;
	DropLoot(entityManager);

	unsigned int playerID = 0;
	if (!CharacterControllerSystem::TryFindPlayer(entityManager, playerID) || BonfireSystem::IsPlayerResting(entityManager)
	    || MountSystem::IsRiding(entityManager, playerID))
		return;
	const TransformComponent* player = entityManager.GetComponent<TransformComponent>(playerID);
	const HealthComponent* health = entityManager.GetComponent<HealthComponent>(playerID);
	if (!player || (health && health->isDead))
		return;

	// The nearest pickup in reach is the one E takes
	PickupComponent* nearest = nullptr;
	unsigned int nearestID = 0;
	float nearestDistance = 1e9f;
	for (PickupComponent& pickup : entityManager.GetAllComponents<PickupComponent>())
	{
		if (pickup.isTaken)
			continue;
		const unsigned int pickupID = entityManager.GetEntityIDForComponent(pickup);
		const TransformComponent* transform = entityManager.GetComponent<TransformComponent>(pickupID);
		if (!transform)
			continue;
		const glm::vec3 offset = transform->position - player->position;
		const float distance = glm::length(KeepOnlyHorizontalPart(offset));
		if (distance <= pickup.interactDistance && std::abs(offset.y) < 2.5f && distance < nearestDistance)
		{
			nearest = &pickup;
			nearestID = pickupID;
			nearestDistance = distance;
		}
	}
	if (!nearest)
		return;

	HUDSystem::ShowPrompt(entityManager, nearest->prompt);
	if (InputManager::IsKeyDown(PickupKey))
		Take(entityManager, *nearest, nearestID, playerID);
}

void PickupSystem::AddItem(EntityManager& entityManager, unsigned int ownerID, const std::string& itemId)
{
	InventoryComponent* inventory = entityManager.GetComponent<InventoryComponent>(ownerID);
	if (!inventory)
		inventory = &entityManager.AddComponent<InventoryComponent>(ownerID);
	inventory->items.push_back(itemId);
}

bool PickupSystem::RemoveItem(EntityManager& entityManager, unsigned int ownerID, const std::string& itemId)
{
	InventoryComponent* inventory = entityManager.GetComponent<InventoryComponent>(ownerID);
	if (!inventory)
		return false;
	auto found = std::find(inventory->items.begin(), inventory->items.end(), itemId);
	if (found == inventory->items.end())
		return false;
	inventory->items.erase(found);
	return true;
}

int PickupSystem::CountItem(EntityManager& entityManager, unsigned int ownerID, const std::string& itemId)
{
	const InventoryComponent* inventory = entityManager.GetComponent<InventoryComponent>(ownerID);
	return inventory ? static_cast<int>(std::count(inventory->items.begin(), inventory->items.end(), itemId)) : 0;
}

}
