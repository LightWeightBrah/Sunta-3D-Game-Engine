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
	// With hot reload on, a script file changed on the disk is loaded again (the editor and a development build want it, a game for players doesn't)
	static void UpdateScripts(EntityManager& entityManager, float deltaTime, bool isHotReloadEnabled);

	static void DispatchTriggerEnter(EntityManager& entityManager, unsigned int triggerEntityID, unsigned int otherEntityID);
	static void DispatchTriggerStay(EntityManager& entityManager,  unsigned int triggerEntityID, unsigned int otherEntityID);
	static void DispatchTriggerExit(EntityManager& entityManager,  unsigned int triggerEntityID, unsigned int otherEntityID);

	static void DispatchCollisionEnter(EntityManager& entityManager, unsigned int collisionEntityID, unsigned int otherEntityID);
	static void DispatchCollisionStay(EntityManager& entityManager,  unsigned int collisionEntityID, unsigned int otherEntityID);
	static void DispatchCollisionExit(EntityManager& entityManager,  unsigned int collisionEntityID, unsigned int otherEntityID);

};


}