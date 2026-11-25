#pragma once

#include <string>
#include <vector>

namespace Sunta
{

class Scene;
class EntityManager;

class SceneSerializer
{
public:
	static bool Serialize(const std::string& filepath, Scene& scene);

	// The text that Serialize() writes to a file (the editor compares it to find unsaved changes)
	static std::string SerializeToString(Scene& scene);

	static bool Deserialize(const std::string& filepath, Scene& scene);

	// Copies an entity and everything under it (its children, theirs...) as new entities, exactly as saving and loading
	// a scene would (a template of the scene spawned while the game plays: see Lua world:spawn). The copy of 'entityID'
	// has no parent. False when the entity doesn't exist
	static bool CloneWithDescendants(EntityManager& entityManager, unsigned int entityID, unsigned int& outCopyID,
		std::vector<unsigned int>* outAllCopies = nullptr);
};

}
