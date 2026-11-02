#pragma once

#include <cmath>
#include <string>
#include <glm/glm.hpp>

#include "ECS/ComponentLayout.h"
#include "Core/EngineAssets.h"

namespace Sunta
{

// Raising the character's level with the Currency while resting at a bonfire (leveling up in Dark Souls, Elden Ring...)
//
// How to use it: Add Component > Level Up on the player (it needs Currency to pay with). While the player rests at a bonfire
// a menu shows the level and the attributes as in Dark Souls, each with its value now and after the level up ("10 -> 11"):
// W/S (or up/down) chooses an attribute, D/A (or right/left) adds or takes back a point, Enter pays and keeps them all
// Leaving the bonfire forgets the points not confirmed. Every level costs more: the price starts at 'firstLevelCost'
// and grows 'costGrowth' times with every level
// There are three attributes, each with its own effect in the engine and a name the game gives it ('displayName'):
//  - health:  'healthPerPoint' more maximum health per point (Vigor)
//  - stamina: 'staminaPerPoint' more maximum stamina per point (Endurance)
//  - damage:  'damagePerPoint' more weapon damage per point, 0.05 = +5% (Strength)
// An attribute with an empty name is left out of the menu. The menu shows each one as 'startingAttributeValue' + the points
// The points are saved with the scene: the Health and Stamina in the scene are the values without them
// 'levelUpSound' plays and 'levelUpEffect' (a child entity with a Particle Emitter) bursts with every raised level
// A script can use it: this.levelUp.level, this.levelUp.cost, this.levelUp:raise("health"|"stamina"|"damage")
struct LevelUpComponent
{
	// Limits of the sliders in the Inspector
	static constexpr float MaxPointGain   = 100.0f;
	static constexpr float MaxDamageGain  = 1.0f;
	static constexpr float MaxCostGrowth  = 2.0f;

	std::string displayName = "Level";
	int         level       = 1;

	int   firstLevelCost = 200;
	float costGrowth     = 1.12f;

	int startingAttributeValue = 10;

	std::string healthAttribute = "Vigor";
	float       healthPerPoint  = 10.0f;
	int         healthPoints    = 0;

	std::string staminaAttribute = "Endurance";
	float       staminaPerPoint  = 8.0f;
	int         staminaPoints    = 0;

	std::string damageAttribute = "Strength";
	float       damagePerPoint  = 0.05f;
	int         damagePoints    = 0;

	std::string levelUpSound;
	std::string levelUpEffect;

	// ========== Filled while the game plays, not saved with the scene ==========

	// The maximum health and stamina from the scene, before the points are added
	float baseMaxHealth     = 0.0f;
	float baseMaxStamina    = 0.0f;
	bool  isBaseRemembered  = false;

	// The row chosen in the menu, the points added in the menu but not confirmed yet, and how long the menu still glows
	// after a confirmed level up
	int   selectedAttribute     = 0;
	int   pendingHealthPoints   = 0;
	int   pendingStaminaPoints  = 0;
	int   pendingDamagePoints   = 0;
	float raisedGlowSecondsLeft = 0.0f;

	// The price of reaching the level after 'fromLevel'
	int GetCostAt(int fromLevel) const
	{
		return (int)std::round(firstLevelCost * std::pow(costGrowth, (float)glm::max(fromLevel - 1, 0)));
	}

	// The price of the next level
	int GetCost() const { return GetCostAt(level); }

	int GetPendingPoints() const { return pendingHealthPoints + pendingStaminaPoints + pendingDamagePoints; }

	// The price of all the points added in the menu, level after level
	int GetPendingCost() const
	{
		int cost = 0;
		for (int i = 0; i < GetPendingPoints(); ++i)
			cost += GetCostAt(level + i);
		return cost;
	}

	float GetDamageMultiplier() const { return 1.0f + damagePoints * damagePerPoint; }

	static void RegisterToInspector()
	{
		InspectorComponentRegistry::RegisterComponent<LevelUpComponent>("Level Up", EngineAssets::Icons::SoulsBonfire,
			{
				ADD_PROPERTY(LevelUpComponent, displayName, PropertyDataType::String),
				ADD_PROPERTY(LevelUpComponent, level, PropertyDataType::Int),
				ADD_PROPERTY(LevelUpComponent, firstLevelCost, PropertyDataType::Int),
				ADD_PROPERTY_RANGED(LevelUpComponent, costGrowth, PropertyDataType::Float, 1.0f, MaxCostGrowth),
				ADD_PROPERTY(LevelUpComponent, startingAttributeValue, PropertyDataType::Int),
				ADD_PROPERTY(LevelUpComponent, healthAttribute, PropertyDataType::String),
				ADD_PROPERTY_RANGED(LevelUpComponent, healthPerPoint, PropertyDataType::Float, 0.0f, MaxPointGain),
				ADD_PROPERTY(LevelUpComponent, healthPoints, PropertyDataType::Int),
				ADD_PROPERTY(LevelUpComponent, staminaAttribute, PropertyDataType::String),
				ADD_PROPERTY_RANGED(LevelUpComponent, staminaPerPoint, PropertyDataType::Float, 0.0f, MaxPointGain),
				ADD_PROPERTY(LevelUpComponent, staminaPoints, PropertyDataType::Int),
				ADD_PROPERTY(LevelUpComponent, damageAttribute, PropertyDataType::String),
				ADD_PROPERTY_RANGED(LevelUpComponent, damagePerPoint, PropertyDataType::Float, 0.0f, MaxDamageGain),
				ADD_PROPERTY(LevelUpComponent, damagePoints, PropertyDataType::Int),
				ADD_PROPERTY(LevelUpComponent, levelUpSound, PropertyDataType::String),
				ADD_PROPERTY(LevelUpComponent, levelUpEffect, PropertyDataType::String)
			});
	}
};

}
