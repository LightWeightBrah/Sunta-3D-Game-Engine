#pragma once

#include <string>
#include <vector>

#include "ECS/ComponentLayout.h"
#include "Core/EngineAssets.h"

namespace Sunta
{

// Something lying in the world the player can pick up: an item, a key, a rune, a page or a book whose text is read
// aloud (the lore of souls-likes, the books of Diablo)
//
// How to use it: Add Component > Pickup on an entity with a Model (and a Particle Emitter child for its glint). Near
// it the player sees 'prompt' and takes it with E: it disappears, 'itemId' goes into the player's Inventory and
// "Obtained: <displayName>" shows. A book ('readText' set) is read aloud: 'voiceSound' plays over the quieted music
// while the text stays on the screen ('readSeconds', 0 = as long as the voice or the length of the text)
// An enemy drops a copy of a pickup when it dies (see LootDropComponent): the pickup then serves as a template
struct PickupComponent
{
	static constexpr float MaxDistance = 10.0f;
	static constexpr float MaxSeconds  = 120.0f;

	std::string itemId;
	std::string displayName;
	std::string prompt = "Pick up   [E]";
	float       interactDistance = 1.8f;

	// A book or a letter: its words (\n starts a new line), the voice reading them, how long they stay
	std::string readText;
	std::string voiceSound;
	float       readSeconds = 0.0f;

	std::string pickupSound;

	// ========== Filled while the game plays, not saved with the scene ==========

	bool isTaken = false;

	static void RegisterToInspector()
	{
		InspectorComponentRegistry::RegisterComponent<PickupComponent>("Pickup", EngineAssets::Icons::Bonfire,
			{
				ADD_PROPERTY(PickupComponent, itemId,      PropertyDataType::String),
				ADD_PROPERTY(PickupComponent, displayName, PropertyDataType::String),
				ADD_PROPERTY(PickupComponent, prompt,      PropertyDataType::String),
				ADD_PROPERTY_RANGED(PickupComponent, interactDistance, PropertyDataType::Float, 0.0f, MaxDistance),
				ADD_PROPERTY(PickupComponent, readText,    PropertyDataType::String),
				ADD_PROPERTY(PickupComponent, voiceSound,  PropertyDataType::String),
				ADD_PROPERTY_RANGED(PickupComponent, readSeconds, PropertyDataType::Float, 0.0f, MaxSeconds),
				ADD_PROPERTY(PickupComponent, pickupSound, PropertyDataType::String)
			});
	}
};

// What the player carries (see PickupComponent): item ids, one entry per item picked up. Scripts ask it
// (this.inventory:has("crypt_key"), :count("ember")) and change it (:add, :remove)
struct InventoryComponent
{
	std::vector<std::string> items;

	static void RegisterToInspector()
	{
		InspectorComponentRegistry::RegisterComponent<InventoryComponent>("Inventory", EngineAssets::Icons::Bonfire, { });
	}
};

// An enemy that leaves something behind when it dies: a copy of the Pickup entity named 'pickupTemplate' appears
// where it fell (with 'chance', 0..1). 'onlyOnce': a book or a key drops the first time only, not again after the
// enemy came back at a bonfire
struct LootDropComponent
{
	std::string pickupTemplate;
	float       chance   = 1.0f;
	bool        onlyOnce = true;

	// ========== Filled while the game plays, not saved with the scene ==========

	bool hasDroppedThisLife = false;
	bool hasEverDropped     = false;

	static void RegisterToInspector()
	{
		InspectorComponentRegistry::RegisterComponent<LootDropComponent>("Loot Drop", EngineAssets::Icons::Bonfire,
			{
				ADD_PROPERTY(LootDropComponent, pickupTemplate, PropertyDataType::String),
				ADD_PROPERTY_RANGED(LootDropComponent, chance, PropertyDataType::Float, 0.0f, 1.0f),
				ADD_PROPERTY(LootDropComponent, onlyOnce, PropertyDataType::Bool)
			});
	}
};

}
