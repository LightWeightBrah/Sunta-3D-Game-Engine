#pragma once
#include <memory>
#include <glm/glm.hpp>
#include "ComponentLayout.h"

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

struct TransformComponent
{
	glm::vec3 position = glm::vec3(0.0f);
	glm::vec3 rotation = glm::vec3(0.0f);
	glm::vec3 scale	   = glm::vec3(1.0f);

	bool isDirty = true;

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
	std::shared_ptr<Mesh> mesh;
	std::shared_ptr<Material> material;
};

struct BasicLight
{
	glm::vec3 ambientIntensity  = glm::vec3(0.2f);
	glm::vec3 diffuseIntensity  = glm::vec3(0.5f);
	glm::vec3 specularIntensity = glm::vec3(1.0f);
};

struct DirectionalLightComponent : BasicLight
{
	glm::vec3 direction			= glm::vec3(-0.2f, -1.0f, -0.3f);

	static void RegisterToInspector()
	{
		InspectorComponentRegistry::RegisterComponent<DirectionalLightComponent>("Directional Light",
			{
				ADD_PROPERTY(DirectionalLightComponent, ambientIntensity,	PropertyDataType::Color),
				ADD_PROPERTY(DirectionalLightComponent, diffuseIntensity,	PropertyDataType::Color),
				ADD_PROPERTY(DirectionalLightComponent, specularIntensity,	PropertyDataType::Color),

				ADD_PROPERTY(DirectionalLightComponent, direction,			PropertyDataType::Float3)
			});
	}
};

struct PointLightComponent : BasicLight
{
	float constant				= 1.0f;
	float linear				= 0.09f;
	float quadratic				= 0.032f;

	static void RegisterToInspector()
	{
		InspectorComponentRegistry::RegisterComponent<PointLightComponent>("Directional Light",
			{
				ADD_PROPERTY(PointLightComponent, ambientIntensity,		PropertyDataType::Color),
				ADD_PROPERTY(PointLightComponent, diffuseIntensity,		PropertyDataType::Color),
				ADD_PROPERTY(PointLightComponent, specularIntensity,	PropertyDataType::Color),

				ADD_PROPERTY(PointLightComponent, constant,				PropertyDataType::Float),
				ADD_PROPERTY(PointLightComponent, linear,				PropertyDataType::Float),
				ADD_PROPERTY(PointLightComponent, quadratic,			PropertyDataType::Float)
			});
	}
};

struct SpotlightComponent : BasicLight
{
	float cutOffAngle		= 12.5f;
	float outerCutOffAngle	= 17.5f;

	float constant			= 1.0f;
	float linear			= 0.09f;
	float quadratic			= 0.032f;

	static void RegisterToInspector()
	{
		InspectorComponentRegistry::RegisterComponent<SpotlightComponent>("Directional Light",
			{
				ADD_PROPERTY(SpotlightComponent, ambientIntensity,		PropertyDataType::Color),
				ADD_PROPERTY(SpotlightComponent, diffuseIntensity,		PropertyDataType::Color),
				ADD_PROPERTY(SpotlightComponent, specularIntensity,		PropertyDataType::Color),

				ADD_PROPERTY(SpotlightComponent, cutOffAngle,			PropertyDataType::Float),
				ADD_PROPERTY(SpotlightComponent, outerCutOffAngle,		PropertyDataType::Float),

				ADD_PROPERTY(SpotlightComponent, constant,				PropertyDataType::Float),
				ADD_PROPERTY(SpotlightComponent, linear,				PropertyDataType::Float),
				ADD_PROPERTY(SpotlightComponent, quadratic,				PropertyDataType::Float)
			});
	}
};


}