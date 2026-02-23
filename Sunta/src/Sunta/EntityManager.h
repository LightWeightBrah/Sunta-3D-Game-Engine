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
};

template<typename T>
struct ComponentStorage : public IInspectableStorage
{
	std::vector<T>   components;
	std::vector<int> entityToComponent;

	T& Assign(unsigned int entityID, T&& data)
	{
		if (entityID >= entityToComponent.size())
			entityToComponent.resize(entityID + 1, -1);
		
		entityToComponent[entityID] = components.size();
		components.push_back(std::move(data));
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
};

class EntityManager
{
public:
	unsigned int CreateEntity()
	{
		return nextID++; //first returns nextID then increments it
	}

	template<typename T, typename... Args>
	T& AddComponent(unsigned int entityID, Args&&... args)
	{
		auto& storage = GetOrCreateStorage<T>();

		//register component for the editor the first time we add new component type
		inspectableMap.try_emplace(typeid(T).hash_code(), &storage);

		return storage.Assign(entityID, T{ std::forward<Args>(args)... });
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

	const std::unordered_map<size_t, IInspectableStorage*>& GetInspectableMap() const
	{
		return inspectableMap;
	}

private:
	unsigned int nextID = 0;
	std::unordered_map<size_t, IInspectableStorage*> inspectableMap;

	template<typename T>
	ComponentStorage<T>& GetOrCreateStorage()
	{
		static ComponentStorage<T> instance;
		return instance;
	}
};


}
