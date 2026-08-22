#pragma once

namespace Sunta
{

class EntityManager;

class Systems
{
public:
	static void UpdateTransform(EntityManager& entityManager);
	static void SyncMeshComponents(EntityManager& entityManager);
	static void SyncModelComponents(EntityManager& entityManager);
	static void UpdateAnimators(EntityManager& entityManager, float deltaTime);
	static void UpdateScripts(EntityManager& entityManager, float deltaTime);
};


}