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

	static void DispatchTriggerEnter(EntityManager& entityManager, unsigned int triggerEntityID, unsigned int otherEntityID);
	static void DispatchTriggerStay(EntityManager& entityManager,  unsigned int triggerEntityID, unsigned int otherEntityID);
	static void DispatchTriggerExit(EntityManager& entityManager,  unsigned int triggerEntityID, unsigned int otherEntityID);

	static void DispatchCollisionEnter(EntityManager& entityManager, unsigned int collisionEntityID, unsigned int otherEntityID);
	static void DispatchCollisionStay(EntityManager& entityManager,  unsigned int collisionEntityID, unsigned int otherEntityID);
	static void DispatchCollisionExit(EntityManager& entityManager,  unsigned int collisionEntityID, unsigned int otherEntityID);

};


}