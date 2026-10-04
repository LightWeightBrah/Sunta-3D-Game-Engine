#pragma once

#include <utility>

#include "ECS/EntityManager.h"

namespace Sunta
{

class Entity
{
public:
	Entity() = default;
	Entity(unsigned int entityID, EntityManager* entityManager)
		: entityID(entityID)
		, entityManager(entityManager) { }

	unsigned int GetID() const { return entityID; }

	// False for a default-constructed Entity or one that has been destroyed
	bool IsValid() const { return entityManager && entityManager->IsAlive(entityID); }

	template<typename T, typename... Args>
	T& AddComponent(Args&&... args)
	{
		return entityManager->AddComponent<T>(entityID, std::forward<Args>(args)...);
	}

	// Returns nullptr when the entity doesn't have this component
	template<typename T>
	T* GetComponent() const
	{
		return entityManager ? entityManager->GetComponent<T>(entityID) : nullptr;
	}

	template<typename T>
	bool HasComponent() const 
	{ 
		return GetComponent<T>() != nullptr; 
	}

private:
	unsigned int entityID		 = 0;
	EntityManager* entityManager = nullptr;
};


}