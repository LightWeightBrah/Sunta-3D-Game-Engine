#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <limits>

namespace Sunta
{

enum class PropertyDataType
{
	Int,
	Bool,
	Float,
	Float3,
	Color,
	String,
	AssetPath
};

enum class AssetType
{
	None,
	Mesh,
	Texture,
	Material,
	Shader
};

struct PropertyDefinition
{
	std::string label;
	unsigned int byteOffset;
	PropertyDataType dataType;

	// Metadata for assets
	AssetType assetType = AssetType::None;

	float minValue = -std::numeric_limits<float>::infinity();
	float maxValue =  std::numeric_limits<float>::infinity();

	bool HasRange() const
	{
		return minValue != -std::numeric_limits<float>::infinity() &&
			   maxValue !=  std::numeric_limits<float>::infinity();
	}
};

struct ComponentType
{
	std::string name;
	std::vector<PropertyDefinition> properties;
	std::function<void(void*)> onChanged = nullptr;
};

#define ADD_PROPERTY(ComponentStruct, PropertyName, PropertyType) \
	{ #PropertyName, (unsigned int)offsetof(ComponentStruct, PropertyName), PropertyType}

#define ADD_PROPERTY_RANGED(ComponentStruct, PropertyName, PropertyType, MinValue, MaxValue) \
	{ #PropertyName, (unsigned int)offsetof(ComponentStruct, PropertyName), PropertyType, MinValue, MaxValue }

#define ADD_ASSET(ComponentStruct, PropertyName, AssetTypeValue) \
	{ #PropertyName, (unsigned int)offsetof(ComponentStruct, PropertyName), PropertyDataType::AssetPath, AssetTypeValue }


class InspectorComponentRegistry
{
public:
	template<typename T>
	static void RegisterComponent(const std::string& name, std::vector<PropertyDefinition> properties, std::function<void(void*)> onChanged = nullptr)
	{
		ComponentType newComponentType;

		newComponentType.name = name;
		newComponentType.properties = properties;
		newComponentType.onChanged = onChanged;

		GetComponentsMap()[typeid(T).hash_code()] = newComponentType;
	}

	static const ComponentType* GetComponentTypeByHash(size_t hash)
	{
		auto& componentsMap = GetComponentsMap();
		
		auto it = componentsMap.find(hash);
		if (it != componentsMap.end())
			return &it->second;

		return nullptr;
	}

private:
	static std::unordered_map<size_t, ComponentType>& GetComponentsMap()
	{
		static std::unordered_map<size_t, ComponentType> componentsMap;
		return componentsMap;
	}
};


}