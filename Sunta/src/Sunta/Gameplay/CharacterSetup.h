#pragma once

#include <string>
#include <glm/glm.hpp>

namespace Sunta
{

class EntityManager;

// What every ready sword fighter is made of (the Souls Player and the enemies share it)
// Every function leaves the parts the entity already has as they are
class CharacterSetup
{
public:
	// An entity with a name and a place in the world (Tag, Transform, World Matrix)
	static unsigned int CreateNamedEntity(EntityManager& entityManager, const std::string& name, const glm::vec3& position, const glm::vec3& rotation);

	// The model is loaded at once (not on the next frame), so the components added after it can pick its animations
	static void AddModelIfMissing(EntityManager& entityManager, unsigned int entityID, const std::string& modelName);

	// Character Controller, Character Animation, Character Actions and Health
	static void AddFighterComponents(EntityManager& entityManager, unsigned int entityID);

	// A sword as a child on the weapon bone of the Knight model, with a Hitbox that hurts the colliders the given layer may hit
	// Only for a model with that bone: on another model nobody knows where the hand is
	static void AddSwordIfPossible(EntityManager& entityManager, unsigned int entityID, const std::string& hitboxLayerName, float damage);

	// Moves the body (the Box Collider) to the named collision layer, when the project has a layer with that name
	static void SetBodyLayer(EntityManager& entityManager, unsigned int entityID, const std::string& layerName);
};

}
