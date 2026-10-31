#pragma once

namespace Sunta
{

class EntityManager;

// Turns one entity into the Rusted Knight, a ready enemy: a rusted knight that fights with a sword (a Melee Enemy)
// In the editor: Entity > Create Rusted Knight, the toolbar, or Add Component > Rusted Knight
//
// It adds, leaving the parts the entity already has as they are: the Rusted Knight model (when the entity has no Model),
// Character Controller (driven by the enemy, not the keyboard), Character Animation, Character Actions, Health,
// Enemy + Melee Enemy (the states), Lock On (to face the player), Lock On Target (so the player can lock on to it)
// and a sword with a Hitbox in its hand
class RustedKnightSetup
{
public:
	static void SetUp(EntityManager& entityManager, unsigned int entityID);

	// True when the entity already is an enemy (it has Enemy)
	static bool IsSetUp(EntityManager& entityManager, unsigned int entityID);
};

}
