#pragma once

namespace Sunta
{

class EntityManager;

// Calls, keeps and sends away the allies of the player (see SummonComponent)
class SummonSystem
{
public:
	// Once a frame while the game plays: G calls the allies when there is a charge left, the called ones fade away
	// when their time is up or a while after they fell, a rest at a bonfire refills the charges and sends them away
	static void Update(EntityManager& entityManager, float deltaTime);
};

}
