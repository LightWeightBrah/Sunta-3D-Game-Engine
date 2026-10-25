#pragma once

#include <string>

namespace Sunta
{

class EntityManager;

// The name of the entity for messages in the log, a placeholder when the entity has no Tag
std::string GetEntityName(EntityManager& entityManager, unsigned int entityID);

}
