#pragma once

namespace Sunta
{

class EntityManager;

// Turns one entity into a complete Dark Souls style player in one click (editor: Add Component > Souls Player,
// or Entity > Create Souls Player)
//
// It adds everything a player needs, leaving the parts the entity already has as they are:
// the Knight model (when the entity has no Model), Character Controller, Character Animation (idle, walk, run),
// Character Actions (roll, backstep, light combo, heavy attack), Lock On,
// a sword held in the right hand (a child entity that follows the weapon bone of the Knight)
// and a Follow Camera (on the camera of the scene, or on a new one)
class SoulsPlayerSetup
{
public:
	static void SetUp(EntityManager& entityManager, unsigned int entityID);

	// True when the entity already is a Souls player (it has Character Actions)
	static bool IsSetUp(EntityManager& entityManager, unsigned int entityID);
};

}
