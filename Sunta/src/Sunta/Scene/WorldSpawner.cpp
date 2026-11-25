#include "Core/SuntaPreCompiled.h"
#include "WorldSpawner.h"

#include <vector>

#include "ECS/EntityManager.h"
#include "ECS/Component.h"
#include "Gameplay/EntityName.h"
#include "Scene/EntityHierarchy.h"
#include "Serialization/SceneSerializer.h"

namespace Sunta
{

namespace
{

std::vector<unsigned int>& GetRequestedForDestroy()
{
	static std::vector<unsigned int> requested;
	return requested;
}

}

bool WorldSpawner::Spawn(EntityManager& entityManager, const std::string& templateName, const glm::vec3& position,
	float yawDegrees, unsigned int& outEntityID)
{
	unsigned int templateID = 0;
	if (!TryFindEntityByName(entityManager, templateName, templateID))
		return false;

	if (!SceneSerializer::CloneWithDescendants(entityManager, templateID, outEntityID))
		return false;

	if (TransformComponent* transform = entityManager.GetComponent<TransformComponent>(outEntityID))
	{
		transform->position = position;
		transform->rotation = glm::vec3(transform->rotation.x, yawDegrees, transform->rotation.z);
		transform->SyncQuaternionFromEuler();
		transform->isDirty = true;
	}
	return true;
}

void WorldSpawner::RequestDestroy(unsigned int entityID)
{
	GetRequestedForDestroy().push_back(entityID);
}

void WorldSpawner::DestroyRequested(EntityManager& entityManager)
{
	std::vector<unsigned int>& requested = GetRequestedForDestroy();
	for (unsigned int entityID : requested)
	{
		if (!entityManager.IsAlive(entityID))
			continue;
		for (unsigned int familyID : EntityHierarchy::GetWithDescendants(entityManager, entityID))
			entityManager.DestroyEntity(familyID);
	}
	requested.clear();
}

}
