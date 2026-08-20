#pragma once
#include <unordered_map>
#include <memory>

#include "ECS/IComponentDrawer.h"
#include "ECS/Component.h"
#include "Drawers/ScriptComponentDrawer.h"

namespace Sunta
{

class ComponentDrawerFactory
{
public:
	static void Init()
	{
		RegisterDrawer<ScriptComponent, ScriptComponentDrawer>();
	}

	template<typename T, typename DrawerType>
	static void RegisterDrawer()
	{
		GetDrawersMap()[typeid(T).hash_code()] = std::make_unique<DrawerType>();
	}

	static IComponentDrawer* GetDrawer(size_t typeHash)
	{
		auto& drawers = GetDrawersMap();
		auto it = drawers.find(typeHash);
		return (it != drawers.end()) ? it->second.get() : nullptr;
	}

private:
	static std::unordered_map<size_t, std::unique_ptr<IComponentDrawer>>& GetDrawersMap()
	{
		static std::unordered_map<size_t, std::unique_ptr<IComponentDrawer>> drawers;
		return drawers;
	}

};

}