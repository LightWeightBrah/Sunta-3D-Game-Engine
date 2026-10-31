#pragma once

#include <vector>
#include <glm/glm.hpp>

#include "ECS/ComponentLayout.h"
#include "Core/EngineAssets.h"
#include "Physics/CollisionLayers.h"

namespace Sunta
{

// The part of a weapon that hurts: a box that deals damage while its owner swings it
//
// How to use it: put it on the weapon, a child entity of the character (the Souls Player and the enemies get it on their sword)
// The owner is the closest parent with Character Actions. The box hurts only during the striking part of an attack
// and hits every target once per swing. A heavy attack multiplies the damage (Character Actions)
// Targets are entities with Health and a Box Collider, and the collision rules decide who can be hit:
// the 'layer' of the hitbox against the layer of the target's collider (by default PlayerHitbox hits Enemy, EnemyHitbox hits Player)
struct HitboxComponent
{
	// Limit of the slider in the Inspector
	static constexpr float MaxDamage = 1000.0f;

	float damage = 25.0f;

	// The box, in the space of the weapon (the Knight's sword: the blade goes up along +Y from the grip)
	glm::vec3 localOffset = glm::vec3(0.0f, 0.56f, 0.0f);
	glm::vec3 halfExtents = glm::vec3(0.06f, 0.48f, 0.06f);

	CollisionLayer layer = DefaultCollisionLayer;

	// Draws this box even when View > Show Weapon Hitboxes is off
	bool showGizmos = false;

	// Who was already hit during the current swing (and which swing that is). Not saved with the scene
	std::vector<unsigned int> entitiesHitThisSwing;
	unsigned int swingNumber = 0;

	// Above zero for a moment after the weapon hurt someone (the box is drawn red then). Not saved with the scene
	float hitFlashSecondsLeft = 0.0f;

	static void RegisterToInspector()
	{
		InspectorComponentRegistry::RegisterComponent<HitboxComponent>("Hitbox", EngineAssets::Icons::BoxCollider,
			{
				ADD_PROPERTY_RANGED(HitboxComponent, damage, PropertyDataType::Float, 0.0f, MaxDamage),
				ADD_PROPERTY(HitboxComponent, localOffset, PropertyDataType::Float3),
				ADD_PROPERTY(HitboxComponent, halfExtents, PropertyDataType::Float3),
				ADD_PROPERTY(HitboxComponent, layer,       PropertyDataType::CollisionLayer),
				ADD_PROPERTY(HitboxComponent, showGizmos,  PropertyDataType::Bool)
			});
	}
};

}
