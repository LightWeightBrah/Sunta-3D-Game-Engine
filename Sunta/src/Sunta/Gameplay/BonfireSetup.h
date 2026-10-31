#pragma once

namespace Sunta
{

class EntityManager;

// Turns one entity into a ready bonfire (editor: Entity > Create Bonfire, the toolbar, or Add Component > Bonfire Setup):
// the bonfire model, the Bonfire component, a small Box Collider (the player can't walk through the fire)
// and a warm Point Light as a child entity, just above the flames
class BonfireSetup
{
public:
	static void SetUp(EntityManager& entityManager, unsigned int entityID);

	// True when the entity already is a bonfire
	static bool IsSetUp(EntityManager& entityManager, unsigned int entityID);
};

}
