#pragma once
#include <vector>
#include <utility>
#include <unordered_map>

namespace Sunta
{

struct IInspectableStorage
{
	virtual ~IInspectableStorage() = default;
	virtual void* GetEntityComponentData(unsigned int entityID) = 0;
	virtual bool Contains(unsigned int entityID) = 0;
	virtual void Remove(unsigned int entityID) = 0;
	virtual void Clear() = 0;
};

template<typename T>
struct ComponentStorage : public IInspectableStorage
{
	std::vector<T>			  components;
	std::vector<int>		  entityToComponent;
	std::vector<unsigned int> componentToEntity;

	template<typename... Args>
	T& Assign(unsigned int entityID, Args&&... args)
	{
		if (entityID >= entityToComponent.size())
			entityToComponent.resize(entityID + 1, -1);
		
		entityToComponent[entityID] = static_cast<int>(components.size());
		componentToEntity.push_back(entityID);
		components.emplace_back(std::forward<Args>(args)...); //emplace_back with forward so we can call constructor in vector
		return components.back();
	}

	T* Get(unsigned int entityID)
	{
		if (Contains(entityID))
			return &components[entityToComponent[entityID]];

		return nullptr;
	}

	void* GetEntityComponentData(unsigned int entityID) override
	{
		return (void*)Get(entityID);
	}

	bool Contains(unsigned int entityID) override
	{
		return (entityID < entityToComponent.size() && entityToComponent[entityID] != -1);
	}

	void Remove(unsigned int entityID) override
	{
		if (!Contains(entityID))
			return;

		int indexToRemove = entityToComponent[entityID];
		int lastIndex = static_cast<int>(components.size()) - 1;

		if (indexToRemove != lastIndex)
		{
			components[indexToRemove] = std::move(components[lastIndex]);
			componentToEntity[indexToRemove] = componentToEntity[lastIndex];

			entityToComponent[componentToEntity[indexToRemove]] = indexToRemove;
		}

		components.pop_back();
		componentToEntity.pop_back();
		entityToComponent[entityID] = -1;
	}

	void Clear() override
	{
		components.clear();
		entityToComponent.clear();
		componentToEntity.clear();
	}
};

class EntityManager
{
public:
	unsigned int CreateEntity()
	{
		unsigned int id = nextID++; //first returns nextID then increments it

		if (id >= aliveEntities.size())
			aliveEntities.resize(id + 1, false);

		aliveEntities[id] = true;
		return id;
	}

	bool IsAlive(unsigned int entityID) const
	{
		return entityID < aliveEntities.size() && aliveEntities[entityID];
	}

	void DestroyEntity(unsigned int entityID)
	{
		for (auto& [hash, storage] : inspectableMap)
		{
			if (storage)
				storage->Remove(entityID);
		}

		if (entityID < aliveEntities.size())
			aliveEntities[entityID] = false;
	}

	template<typename T, typename... Args>
	T& AddComponent(unsigned int entityID, Args&&... args)
	{
		auto& storage = GetOrCreateStorage<T>();

		//register component for the editor the first time we add new component type
		inspectableMap.try_emplace(typeid(T).hash_code(), &storage);

		return storage.Assign(entityID, std::forward<Args>(args)...);
	}

	template<typename T>
	void RemoveComponent(unsigned int entityID)
	{
		GetOrCreateStorage<T>().Remove(entityID);
	}

	template<typename T>
	T* GetComponent(unsigned int entityID)
	{
		return GetOrCreateStorage<T>().Get(entityID);
	}

	template<typename T>
	std::vector<T>& GetAllComponents()
	{
		return GetOrCreateStorage<T>().components;
	}

	unsigned int GetEntityCount() const
	{
		return nextID;
	}

	void Clear()
	{
		nextID = 0;
		aliveEntities.clear();

		for (auto& [hash, storage] : inspectableMap)
		{
			if (storage)
				storage->Clear();
		}
	}

	const std::unordered_map<size_t, IInspectableStorage*>& GetInspectableMap() const
	{
		return inspectableMap;
	}

private:
	unsigned int nextID = 0;
	std::vector<bool> aliveEntities;
	std::unordered_map<size_t, IInspectableStorage*> inspectableMap;

	template<typename T>
	ComponentStorage<T>& GetOrCreateStorage()
	{
		static ComponentStorage<T> instance;
		return instance;
	}
};


}
