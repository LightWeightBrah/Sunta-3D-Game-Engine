#pragma once

#include <string>

namespace Sunta
{

class EntityManager;

// The name of the entity for messages in the log, a placeholder when the entity has no Tag
std::string GetEntityName(EntityManager& entityManager, unsigned int entityID);

// The first entity with this name (its Tag). False for an empty name or when there is none
bool TryFindEntityByName(EntityManager& entityManager, const std::string& name, unsigned int& outEntityID);

}
