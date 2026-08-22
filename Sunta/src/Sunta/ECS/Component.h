#pragma once
#include <memory>
#include <glm/glm.hpp>
#include <sol/sol.hpp>
#include "ComponentLayout.h"

#include "Core/EngineAssets.h"
#include "Core/Log.h"
#include "Renderer/LightingCommon.h"
#include "Scripting/ScriptingEngine.h"
#include "Core/ResourceManager.h"
#include "Animation/Animator.h"

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
	std::string scriptPath;
	std::time_t lastWriteTime = 0;

	sol::environment environment;
	sol::function onCreateFunc;
	sol::function onUpdateFunc;
};

struct ScriptComponent
{
	unsigned int entityID = 0;
	std::vector<ScriptContainer> scripts;

	void LoadScript(const std::string& filepath, unsigned int entityID)
	{
		if (filepath.empty())
			return;

		this->entityID = entityID;

		ScriptContainer& container = scripts.emplace_back();
		container.scriptPath = filepath;

		ReloadScript(container, entityID);
	}

	void ReloadScript(ScriptContainer& container, unsigned int entityID)
	{
		using namespace Scripting;

		if (container.scriptPath.empty())
			return;

		auto& luaState = ScriptingEngine::GetState();

		// Create separated environment, so that every script we attach (e.g on enemy, player) have their own variables etc.
		sol::environment scriptEnvironment(luaState, sol::create, luaState.globals());

		// Add entityID to Lua script so script knows what entity its using
		scriptEnvironment[Fields::EntityID] = entityID;
		
		// Load Lua text file
		sol::protected_function_result result = luaState.script_file(container.scriptPath, scriptEnvironment);
		if (!result.valid())
		{
			sol::error error = result;
			SUNTA_ENGINE_LOG_ERROR("Error Loading Lua script '{0}': '{1}'", container.scriptPath, error.what());
			return;
		}

		container.environment = scriptEnvironment;

		if (std::filesystem::exists(container.scriptPath))
			container.lastWriteTime = std::filesystem::last_write_time(container.scriptPath).time_since_epoch().count();

		// assign OnCreate to container if it exists in Lua
		if (scriptEnvironment[Functions::OnCreate].is<sol::function>())
		{
			container.onCreateFunc = scriptEnvironment[Functions::OnCreate];
			container.onCreateFunc();
		}

		// assign OnUpdate to container if it exists in Lua
		if (scriptEnvironment[Functions::OnUpdate].is<sol::function>())
		{
			container.onUpdateFunc = scriptEnvironment[Functions::OnUpdate];
		}

	}

	static void RegisterToInspector()
	{
		InspectorComponentRegistry::RegisterComponent<ScriptComponent>("Script", EngineAssets::Icons::CppFile,{ });
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

struct ModelComponent
{
	static constexpr const char* NULL_ASSET_NAME = "None";

	unsigned int entityID = 0;
	bool isDirty = true;
	bool isVisible = true;

	std::string modelName = NULL_ASSET_NAME;
	std::shared_ptr<ModelData> modelData;

	ModelComponent(const std::string& modelName)
		: modelName(modelName) { }

	ModelComponent() = default;

	static void RegisterToInspector()
	{
		InspectorComponentRegistry::RegisterComponent<ModelComponent>("Model", EngineAssets::Icons::Skeleton,
			{
				ADD_PROPERTY(ModelComponent, isVisible, PropertyDataType::Bool),
				ADD_ASSET(ModelComponent, modelName, AssetType::Model)
			},
			[](void* data)
			{
				static_cast<ModelComponent*>(data)->isDirty = true;
			});
	}
};

struct AnimatorComponent
{
	Animator animator;
	AnimationType currentAnimationType = AnimationType::IDLE;

	static void RegisterToInspector()
	{
		InspectorComponentRegistry::RegisterComponent<AnimatorComponent>("Animator", EngineAssets::Icons::AnimationController, { });
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