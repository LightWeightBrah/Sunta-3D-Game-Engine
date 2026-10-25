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

}
