#pragma once

namespace Sunta
{

class EntityManager;

class Systems
{
public:
	static void UpdateTransform(EntityManager& entityManager);
	static void SyncMeshComponents(EntityManager& entityManager);
};


}