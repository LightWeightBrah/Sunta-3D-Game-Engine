#pragma once
#include <memory>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <sol/sol.hpp>
#include "ComponentLayout.h"

#include "Core/EngineAssets.h"
#include "Core/Log.h"
#include "Renderer/LightingCommon.h"
#include "Scripting/ScriptingEngine.h"
#include "Core/ResourceManager.h"
#include "Animation/Animator.h"
#include "Physics/CollisionShapes.h"
#include "Physics/CollisionLayers.h"

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

	glm::quat rotationQuaternion = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);

	bool isDirty = true;

	TransformComponent(const glm::vec3& position)
		: position(position) { }

	TransformComponent() = default;

	void SyncQuaternionFromEuler()
	{
		rotationQuaternion = glm::quat(glm::radians(rotation));
	}

	// Syncs Euler angles from the quaternion while preventing abrupt 180 degree representation flips
	//
	// Quaternions have multiple equivalent Euler representations. Standard glm::eulerAngles()
	// can flip between them, causing huge visual jumps in inspector values or interpolation
	//
	// We check both candidates, measure their wrapped angle distance to the current rotation,
	// and pick whichever representation is closest
	void SyncEulerFromQuaternion()
	{
		glm::vec3 newRotation = glm::degrees(glm::eulerAngles(rotationQuaternion));

		// Every 3D orientation has exactly TWO unique Euler representations (ignoring 360° wraps)
		// 'newRotation' is the first, and 'flippedAlternative' is the second valid way to write it
		glm::vec3 flippedAlternative = glm::vec3(
			newRotation.x + 180.0f,
			180.0f - newRotation.y,
			newRotation.z + 180.0f
		);

		// glm::mod is floating-point modulo
		// Wrapping deltas into [-180, 180] degrees handles all 360° multiples automatically,
		// so we only ever need to compare these two representations
		auto wrapDelta = [](const glm::vec3& delta)
			{
				return glm::mod(delta + 180.0f, 360.0f) - 180.0f;
			};

		float distToNew     = glm::length(wrapDelta(newRotation - rotation));
		float distToFlipped = glm::length(wrapDelta(flippedAlternative - rotation));

		// Pick whichever representation is closer to the current rotation to maintain continuity
		rotation = (distToFlipped < distToNew) ? flippedAlternative : newRotation;
	}

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
				transform->SyncQuaternionFromEuler();
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

	bool isStarted = false;

	sol::environment environment;
	sol::function onStartFunc;
	sol::function onUpdateFunc;

	sol::function onTriggerEnterFunc;
	sol::function onTriggerStayFunc;
	sol::function onTriggerExitFunc;

	sol::function onCollisionEnterFunc;
	sol::function onCollisionStayFunc;
	sol::function onCollisionExitFunc;
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

		// assign OnStart to container if it exists in Lua
		if (scriptEnvironment[Functions::OnStart].is<sol::function>())
		{
			container.onStartFunc = scriptEnvironment[Functions::OnStart];
			container.onStartFunc();
		}

		// assign OnUpdate to container if it exists in Lua
		if (scriptEnvironment[Functions::OnUpdate].is<sol::function>())
		{
			container.onUpdateFunc = scriptEnvironment[Functions::OnUpdate];
		}

		// Trigger Funcs
		if (scriptEnvironment[Functions::OnTriggerEnter].is<sol::function>())
			container.onTriggerEnterFunc = scriptEnvironment[Functions::OnTriggerEnter];

		if (scriptEnvironment[Functions::OnTriggerStay].is<sol::function>())
			container.onTriggerStayFunc = scriptEnvironment[Functions::OnTriggerStay];

		if (scriptEnvironment[Functions::OnTriggerExit].is<sol::function>())
			container.onTriggerExitFunc = scriptEnvironment[Functions::OnTriggerExit];

		// Collision Funcs
		if (scriptEnvironment[Functions::OnCollisionEnter].is<sol::function>())
			container.onCollisionEnterFunc = scriptEnvironment[Functions::OnCollisionEnter];

		if (scriptEnvironment[Functions::OnCollisionStay].is<sol::function>())
			container.onCollisionStayFunc = scriptEnvironment[Functions::OnCollisionStay];

		if (scriptEnvironment[Functions::OnCollisionExit].is<sol::function>())
			container.onCollisionExitFunc = scriptEnvironment[Functions::OnCollisionExit];

	}

	void InvokeOnTriggerEnter(unsigned int otherEntityID)
	{
		for (auto& script : scripts)
			if (script.onTriggerEnterFunc.valid())
				script.onTriggerEnterFunc(otherEntityID);
	}

	void InvokeOnTriggerStay(unsigned int otherEntityID)
	{
		for (auto& script : scripts)
			if (script.onTriggerStayFunc.valid())
				script.onTriggerStayFunc(otherEntityID);
	}

	void InvokeOnTriggerExit(unsigned int otherEntityID)
	{
		for (auto& script : scripts)
			if (script.onTriggerExitFunc.valid())
				script.onTriggerExitFunc(otherEntityID);
	}

	void InvokeOnCollisionEnter(unsigned int otherEntityID)
	{
		for (auto& script : scripts)
			if (script.onCollisionEnterFunc.valid())
				script.onCollisionEnterFunc(otherEntityID);
	}

	void InvokeOnCollisionStay(unsigned int otherEntityID)
	{
		for (auto& script : scripts)
			if (script.onCollisionStayFunc.valid())
				script.onCollisionStayFunc(otherEntityID);
	}

	void InvokeOnCollisionExit(unsigned int otherEntityID)
	{
		for (auto& script : scripts)
			if (script.onCollisionExitFunc.valid())
				script.onCollisionExitFunc(otherEntityID);
	}

	static void RegisterToInspector()
	{
		InspectorComponentRegistry::RegisterComponent<ScriptComponent>("Script", EngineAssets::Icons::LuaFile,{ });
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
	std::string currentAnimationName;
	bool needsModelSync = true;

	static void RegisterToInspector()
	{
		InspectorComponentRegistry::RegisterComponent<AnimatorComponent>("Animator", EngineAssets::Icons::AnimationController, { });
	}
};

struct BoxColliderComponent
{
	bool showGizmos = false;

	glm::vec3 localOffset = glm::vec3(0.0f);
	glm::vec3 halfExtents = glm::vec3(0.5f);

	bool isTrigger = false;

	CollisionLayer layer        = CollisionLayer::Environment;
	CollisionMask  collidesWith = MakeMask(CollisionLayer::Environment, CollisionLayer::Player, CollisionLayer::Enemy);

	OBB worldOBB;

	static void RegisterToInspector()
	{
		InspectorComponentRegistry::RegisterComponent<BoxColliderComponent>("Box Collider", EngineAssets::Icons::BoxCollider,
			{
				ADD_PROPERTY(BoxColliderComponent, showGizmos,	 PropertyDataType::Bool),
				ADD_PROPERTY(BoxColliderComponent, localOffset,	 PropertyDataType::Float3),
				ADD_PROPERTY(BoxColliderComponent, halfExtents,	 PropertyDataType::Float3),
				ADD_PROPERTY(BoxColliderComponent, isTrigger,	 PropertyDataType::Bool)
			});
	}
};

struct PhysicsBodyComponent
{
	// Kinematic body ISN'T MOVED BY GRAVITY NOR COLLISIONS
	// Kinmeatic body controls its own movement (e.g Player controlled by keyboard input)
	// Kinematic body STILL WORKS WITH COLLISIONS, it's just nothing "pushes it" automatically 
	bool isKinematic = false;
	bool useGravity  = true;

	glm::vec3 velocity        = glm::vec3(0.0f);

	// Angular Velocity         = how the object spins
	// Direction of this vector = the axis it spins around
	// Length of this vector    = spin speed in radians / second
	// E.g. (0, 3.14, 0)		= spinning around the up-axis at half a turn per secound
	glm::vec3 angularVelocity = glm::vec3(0.0f);

	// restitution = how bouncy object is when it hits something
	float mass = 1.0f;
	float restitution = 0.0f; // 0 = no bounce (sandbag), 1 = SUPER BOUNCY (rubber ball)

	// How much the surface resists sliding: 0 = ice (frictionless), 1 = rubber (strong grip)
	// When two bodies touch, their frictions combine as sqrt(frictionA * frictionB)
	float friction = 0.5f;

	// How quickly motion fades away on its own (like air resistance etc.)
	// 0 = nothing slows down, higher = stops sooner
	float linearDamping = 0.05f;
	float angularDamping = 0.05f;

	// Skips all physics updates when true, stopping tiny leftover values 
	// from causing endless jittering and Inspector flickering
	// Without this, a box resting on the floor would keep getting moved by
	// tiny leftover numbers forever
	bool  isSleeping		   = false;
	float timeSpentAlmostStill = 0.0f;

	static void RegisterToInspector()
	{
		InspectorComponentRegistry::RegisterComponent<PhysicsBodyComponent>("Physics Body", EngineAssets::Icons::Physics,
			{
				ADD_PROPERTY(PhysicsBodyComponent, isKinematic,	     PropertyDataType::Bool),
				ADD_PROPERTY(PhysicsBodyComponent, useGravity,	     PropertyDataType::Bool),
				ADD_PROPERTY(PhysicsBodyComponent, velocity,	     PropertyDataType::Float3),
				ADD_PROPERTY(PhysicsBodyComponent, angularVelocity,	 PropertyDataType::Float3),
				ADD_PROPERTY(PhysicsBodyComponent, mass,		     PropertyDataType::Float),
				ADD_PROPERTY(PhysicsBodyComponent, restitution,		 PropertyDataType::Float),
				ADD_PROPERTY(PhysicsBodyComponent, friction,		 PropertyDataType::Float),
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