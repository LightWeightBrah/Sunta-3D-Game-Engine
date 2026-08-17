#pragma once
#include <memory>
#include <glm/glm.hpp>
#include "ComponentLayout.h"

#include "Core/EngineAssets.h"
#include "Renderer/LightingCommon.h"

namespace Sunta
{

class Entity;
class Mesh;
class Material;

struct Component
{
	virtual ~Component() = default;
	Entity* owner = nullptr;
};

struct TagComponent
{
	std::string name = "New Entity";

	static void RegisterToInspector()
	{
		InspectorComponentRegistry::RegisterComponent<TagComponent>("Tag", EngineAssets::Icons::Transform,
			{
				ADD_PROPERTY(TagComponent, name, PropertyDataType::String)
			});
	}
	
};

struct TransformComponent
{
	glm::vec3 position = glm::vec3(0.0f);
	glm::vec3 rotation = glm::vec3(0.0f);
	glm::vec3 scale	   = glm::vec3(1.0f);

	bool isDirty = true;

	TransformComponent(const glm::vec3& position)
		: position(position) { }

	TransformComponent() = default;

	static void RegisterToInspector()
	{
		InspectorComponentRegistry::RegisterComponent<TransformComponent>("Transform", EngineAssets::Icons::Transform,
			{
				ADD_PROPERTY(TransformComponent, position, PropertyDataType::Float3),
				ADD_PROPERTY(TransformComponent, rotation, PropertyDataType::Float3),
				ADD_PROPERTY(TransformComponent, scale,	   PropertyDataType::Float3)
			},
			[](void* data)
			{
				auto* transform = static_cast<TransformComponent*>(data);
				transform->isDirty = true;
			});
	}
};

struct WorldMatrixComponent
{
	glm::mat4 matrix = glm::mat4(1.0f);
};

struct ScriptContainer
{
	std::string name;
	std::shared_ptr<ScriptableEntity> instance;
	std::function<void(ScriptableEntity*, float)> updateFunc;
};

struct ScriptComponent
{
	std::vector<ScriptContainer> scripts;

	template<typename T>
	void Add(const std::string& scriptName, unsigned int entityID, EntityManager* entityManager)
	{
		auto script = std::make_shared<T>();
		script->entityID = entityID;
		script->entityManager = entityManager;
		script->OnCreate();

		ScriptContainer container;
		container.name = scriptName;
		container.instance = script;
		container.updateFunc = [](ScriptableEntity* scriptableEntity, float deltaTime)
			{
				static_cast<T*>(scriptableEntity)->OnUpdate(deltaTime);
			};

		scripts.push_back(container);
	}
};

struct MeshComponent
{
	static constexpr const char* NULL_ASSET_NAME = "None";

	bool isVisible = true;
	std::string meshName = NULL_ASSET_NAME;
	std::string materialName = NULL_ASSET_NAME;

	std::shared_ptr<Mesh> mesh;
	std::shared_ptr<Material> material;

	bool isDirty = true;

	// Constructor for Init via Primive
	MeshComponent(std::shared_ptr<Mesh> mesh, std::shared_ptr<Material> material)
		: mesh(mesh)
		, material(material)
		, isDirty(false) { }

	// Constructor for Init via asset name
	MeshComponent(std::shared_ptr<Mesh> mesh, std::shared_ptr<Material> material, 
		const std::string& meshName, const std::string& materialName)
		: mesh(mesh)
		, material(material)
		, meshName(meshName)
		, materialName(materialName) { }

	MeshComponent() = default;

	static void RegisterToInspector()
	{
		InspectorComponentRegistry::RegisterComponent<MeshComponent>("Mesh", EngineAssets::Icons::Bonfire,
			{
				ADD_PROPERTY(MeshComponent, isVisible, PropertyDataType::Bool),

				ADD_ASSET(MeshComponent, meshName,	   AssetType::Mesh),
				ADD_ASSET(MeshComponent, materialName, AssetType::Material)
			},
			[](void* data)
			{
				auto* component = static_cast<MeshComponent*>(data);
				component->isDirty = true;
			});

	}

};

struct DirectionalLightComponent
{
	LightColor color;

	static void RegisterToInspector()
	{
		InspectorComponentRegistry::RegisterComponent<DirectionalLightComponent>("Directional Light", EngineAssets::Icons::DirectionalLight,
			{
				ADD_PROPERTY(DirectionalLightComponent, color.ambientIntensity,	 PropertyDataType::Color),
				ADD_PROPERTY(DirectionalLightComponent, color.diffuseIntensity,	 PropertyDataType::Color),
				ADD_PROPERTY(DirectionalLightComponent, color.specularIntensity, PropertyDataType::Color),
			});
	}
};

struct PointLightComponent
{
	LightColor  color;
	Attenuation attenuation;

	static void RegisterToInspector()
	{
		InspectorComponentRegistry::RegisterComponent<PointLightComponent>("Point Light", EngineAssets::Icons::PointLight,
			{
				ADD_PROPERTY(PointLightComponent, color.ambientIntensity,	PropertyDataType::Color),
				ADD_PROPERTY(PointLightComponent, color.diffuseIntensity,	PropertyDataType::Color),
				ADD_PROPERTY(PointLightComponent, color.specularIntensity,	PropertyDataType::Color),

				ADD_PROPERTY(PointLightComponent, attenuation.constant,		PropertyDataType::Float),
				ADD_PROPERTY(PointLightComponent, attenuation.linear,		PropertyDataType::Float),
				ADD_PROPERTY(PointLightComponent, attenuation.quadratic,	PropertyDataType::Float)
			});
	}
};

struct SpotLightComponent
{
	LightColor  color;
	Attenuation attenuation;

	float innerCutOffAngle  = 12.5f;
	float outerCutOffAngle	= 17.5f;

	static void RegisterToInspector()
	{
		InspectorComponentRegistry::RegisterComponent<SpotLightComponent>("Spotlight", EngineAssets::Icons::Spotlight,
			{
				ADD_PROPERTY(SpotLightComponent, color.ambientIntensity,	PropertyDataType::Color),
				ADD_PROPERTY(SpotLightComponent, color.diffuseIntensity,	PropertyDataType::Color),
				ADD_PROPERTY(SpotLightComponent, color.specularIntensity,	PropertyDataType::Color),

				ADD_PROPERTY(SpotLightComponent, attenuation.constant,		PropertyDataType::Float),
				ADD_PROPERTY(SpotLightComponent, attenuation.linear,		PropertyDataType::Float),
				ADD_PROPERTY(SpotLightComponent, attenuation.quadratic,		PropertyDataType::Float),

				ADD_PROPERTY(SpotLightComponent, innerCutOffAngle,			PropertyDataType::Float),
				ADD_PROPERTY(SpotLightComponent, outerCutOffAngle,			PropertyDataType::Float)
			});
	}
};


}