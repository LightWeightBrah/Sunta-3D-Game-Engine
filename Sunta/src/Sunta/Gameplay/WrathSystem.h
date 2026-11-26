#pragma once

namespace Sunta
{

class EntityManager;

// Fills, sets loose and drains the Wrath of the characters that have one (see WrathComponent)
class WrathSystem
{
public:
	// Once a frame while the game plays: V sets a full Wrath of the player loose, a burning one counts down, an idle one drains
	static void Update(EntityManager& entityManager, float deltaTime);

	// The fights fill it (called by the CombatSystem): damage dealt with a weapon, damage taken, a deflected blow
	static void OnDamageDealt(EntityManager& entityManager, unsigned int entityID, float damage);
	static void OnDamageTaken(EntityManager& entityManager, unsigned int entityID, float damage);
	static void OnDeflect(EntityManager& entityManager, unsigned int entityID);

	// How much harder the entity's blows land now, and how much faster they fill a posture (1 = not burning)
	static float GetDamageMultiplier(EntityManager& entityManager, unsigned int entityID);
	static float GetPostureMultiplier(EntityManager& entityManager, unsigned int entityID);
};

}
