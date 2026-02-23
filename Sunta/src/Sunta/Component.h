#pragma once
#include <memory>
#include <glm/glm.hpp>
#include "EditorMacros.h"
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

struct LightComponent
{
	glm::vec3 ambientIntensity  = glm::vec3(0.2f);
	glm::vec3 diffuseIntensity  = glm::vec3(0.5f);
	glm::vec3 specularIntensity = glm::vec3(1.0f);

	static void RegisterToInspector()
	{
		InspectorComponentRegistry::RegisterComponent<LightComponent>("Light Source",
			{
				ADD_PROPERTY(LightComponent, ambientIntensity,	PropertyDataType::Color),
				ADD_PROPERTY(LightComponent, diffuseIntensity,	PropertyDataType::Color),
				ADD_PROPERTY(LightComponent, specularIntensity,	PropertyDataType::Color)
			});
	}
};



}