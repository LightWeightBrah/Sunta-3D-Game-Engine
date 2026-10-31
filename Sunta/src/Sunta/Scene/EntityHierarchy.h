#pragma once

#include <string>
#include <vector>
#include <glm/glm.hpp>

namespace Sunta
{

class EntityManager;

// Parents and children of entities (see ParentComponent): who belongs to whom and where everything is in the world
class EntityHierarchy
{
public:
	// False for an entity without a parent (or whose parent no longer exists)
	static bool TryGetParent(EntityManager& entityManager, unsigned int entityID, unsigned int& outParentID);

	static std::vector<unsigned int> GetChildren(EntityManager& entityManager, unsigned int parentID);

	// The entity, its children, their children and so on (parents before their children)
	static std::vector<unsigned int> GetWithDescendants(EntityManager& entityManager, unsigned int entityID);

	// An entity can't be its own parent, nor the parent of an entity that already carries it
	static bool CanSetParent(EntityManager& entityManager, unsigned int childID, unsigned int parentID);

	// The child stays exactly where it is in the world, only its Transform becomes relative to the new parent
	// Returns false (and changes nothing) when CanSetParent says no
	static bool SetParent(EntityManager& entityManager, unsigned int childID, unsigned int parentID);

	// The entity becomes free again, and stays where it is in the world
	static void ClearParent(EntityManager& entityManager, unsigned int childID);

	// What the Transform of the entity is relative to: the world matrix of its parent (and of the parent's bone),
	// or no change at all for an entity without a parent
	static glm::mat4 GetParentSpace(EntityManager& entityManager, unsigned int entityID);

	// Where a bone (or any node) of the entity's Model is in the world right now. False when the Model has no such bone
	static bool TryGetBoneWorldTransform(EntityManager& entityManager, unsigned int entityID, const std::string& boneName, glm::mat4& outTransform);

	// Fills the world matrix of every entity from its Transform, parents before their children
	// Call after the animators, so children on bones follow the pose of this frame
	static void UpdateWorldMatrices(EntityManager& entityManager);
};

}
