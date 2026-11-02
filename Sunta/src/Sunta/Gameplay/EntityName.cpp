#include "Core/SuntaPreCompiled.h"
#include "EntityName.h"

#include "ECS/Component.h"
#include "ECS/EntityManager.h"

namespace Sunta
{

std::string GetEntityName(EntityManager& entityManager, unsigned int entityID)
{
	const TagComponent* tag = entityManager.GetComponent<TagComponent>(entityID);
	return tag ? tag->name : "Unnamed entity";
}

bool TryFindEntityByName(EntityManager& entityManager, const std::string& name, unsigned int& outEntityID)
{
	if (name.empty())
		return false;

	for (TagComponent& tag : entityManager.GetAllComponents<TagComponent>())
	{
		if (tag.name == name)
		{
			outEntityID = entityManager.GetEntityIDForComponent(tag);
			return true;
		}
	}
	return false;
}

}
