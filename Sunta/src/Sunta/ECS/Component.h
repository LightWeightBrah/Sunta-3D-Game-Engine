#pragma once
#include <memory>
#include <glm/glm.hpp>
#include "ComponentLayout.h"

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
		InspectorComponentRegistry::RegisterComponent<TagComponent>("Tag",
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
		InspectorComponentRegistry::RegisterComponent<TransformComponent>("Transform",
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

struct MeshComponent
{
	static constexpr const char* NULL_ASSET_NAME = "None";

	bool isVisible = true;
	std::string meshName = NULL_ASSET_NAME;
	std::string materialName = NULL_ASSET_NAME;

	std::shared_ptr<Mesh> mesh;
	std::shared_ptr<Material> material;

	bool isDirty = true;

	MeshComponent(std::shared_ptr<Mesh> mesh, std::shared_ptr<Material> material)
		: mesh(mesh)
		, material(material) { }

	MeshComponent() = default;

	static void RegisterToInspector()
	{
		InspectorComponentRegistry::RegisterComponent<MeshComponent>("Mesh",
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
		InspectorComponentRegistry::RegisterComponent<DirectionalLightComponent>("Directional Light",
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
		InspectorComponentRegistry::RegisterComponent<PointLightComponent>("Point Light",
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

struct SpotlightComponent
{
	LightColor  color;
	Attenuation attenuation;

	float innerCutOffAngle  = 12.5f;
	float outerCutOffAngle	= 17.5f;

	static void RegisterToInspector()
	{
		InspectorComponentRegistry::RegisterComponent<SpotlightComponent>("Spotlight",
			{
				ADD_PROPERTY(SpotlightComponent, color.ambientIntensity,	PropertyDataType::Color),
				ADD_PROPERTY(SpotlightComponent, color.diffuseIntensity,	PropertyDataType::Color),
				ADD_PROPERTY(SpotlightComponent, color.specularIntensity,	PropertyDataType::Color),

				ADD_PROPERTY(SpotlightComponent, attenuation.constant,		PropertyDataType::Float),
				ADD_PROPERTY(SpotlightComponent, attenuation.linear,		PropertyDataType::Float),
				ADD_PROPERTY(SpotlightComponent, attenuation.quadratic,		PropertyDataType::Float),

				ADD_PROPERTY(SpotlightComponent, innerCutOffAngle,			PropertyDataType::Float),
				ADD_PROPERTY(SpotlightComponent, outerCutOffAngle,			PropertyDataType::Float)
			});
	}
};


}