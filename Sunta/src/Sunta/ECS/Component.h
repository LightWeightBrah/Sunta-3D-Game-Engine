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
#include "ECS/Entity.h"

namespace Sunta
{

class Mesh;
class Material;

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

		// Every 3D orientation has exactly TWO unique Euler representations (ignoring 360 degrees wraps)
		// 'newRotation' is the first, and 'flippedAlternative' is the second valid way to write it
		glm::vec3 flippedAlternative = glm::vec3(
			newRotation.x + 180.0f,
			180.0f - newRotation.y,
			newRotation.z + 180.0f
		);

		// glm::mod is floating-point modulo
		// Wrapping angles into [-180, 180] degrees handles all 360 degrees multiples automatically,
		// so we only ever need to compare these two representations
		auto wrapAngles = [](const glm::vec3& angles)
			{
				return glm::mod(angles + 180.0f, 360.0f) - 180.0f;
			};

		float distToNew     = glm::length(wrapAngles(newRotation        - rotation));
		float distToFlipped = glm::length(wrapAngles(flippedAlternative - rotation));

		// Pick whichever representation is closer to the current rotation to maintain continuity
		glm::vec3 closerRotation = (distToFlipped < distToNew) ? flippedAlternative : newRotation;

		rotation = wrapAngles(closerRotation);
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
	Entity owner;

	sol::environment environment;

	// protected_function = a Lua error comes back as a result we can log,
	// instead of being silently lost
	sol::protected_function onStartFunc;
	sol::protected_function onUpdateFunc;

	sol::protected_function onTriggerEnterFunc;
	sol::protected_function onTriggerStayFunc;
	sol::protected_function onTriggerExitFunc;

	sol::protected_function onCollisionEnterFunc;
	sol::protected_function onCollisionStayFunc;
	sol::protected_function onCollisionExitFunc;

	// Calls one Lua callback. If the script has an error, we log the script, the function and the Lua message
	// (it contains the line number), then disable that callback until the script is saved (hot-reloaded) again
	// Without disabling it, the same error would be logged every frame
	template<typename... Args>
	void CallFunction(sol::protected_function& function, std::string_view functionName, Args&&... args)
	{
		sol::protected_function_result result = function(std::forward<Args>(args)...);
		if (result.valid())
			return;

		sol::error error = result;
		SUNTA_ENGINE_LOG_ERROR("Lua error in '{0}', function {1}: {2}", scriptPath, functionName, error.what());
		SUNTA_ENGINE_LOG_ERROR("{0} is disabled until you save '{1}' again", functionName, scriptPath);

		function = sol::protected_function();
	}
};

struct ScriptComponent
{
	unsigned int entityID = 0;
	std::vector<ScriptContainer> scripts;

	void AddScript(const std::string& filepath)
	{
		if (filepath.empty())
			return;

		ScriptContainer& container = scripts.emplace_back();
		container.scriptPath = filepath;
	}

	void ReloadScript(ScriptContainer& container, const Entity& entity)
	{
		using namespace Scripting;

		if (container.scriptPath.empty())
			return;

		auto& luaState = ScriptingEngine::GetState();

		// Create separated environment, so that every script we attach (e.g on enemy, player) have their own variables
		sol::environment scriptEnvironment(luaState, sol::create, luaState.globals());

		container.owner = entity;

		// Tell the script which entity it belongs to (available in Lua as 'this')
		scriptEnvironment[Fields::This] = entity; // the entity of this script, with its components (this.name, this.transform, ...)


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
			container.CallFunction(container.onStartFunc, Functions::OnStart);
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

		WarnAboutUnknownCallbacks(scriptEnvironment, container.scriptPath);
	}

	void InvokeOnTriggerEnter  (const Entity& other)  { InvokeOnAllScripts(&ScriptContainer::onTriggerEnterFunc,   Scripting::Functions::OnTriggerEnter,   other); }
	void InvokeOnTriggerStay   (const Entity& other)  { InvokeOnAllScripts(&ScriptContainer::onTriggerStayFunc,    Scripting::Functions::OnTriggerStay,    other); }
	void InvokeOnTriggerExit   (const Entity& other)  { InvokeOnAllScripts(&ScriptContainer::onTriggerExitFunc,    Scripting::Functions::OnTriggerExit,    other); }

	void InvokeOnCollisionEnter(const Entity& other)  { InvokeOnAllScripts(&ScriptContainer::onCollisionEnterFunc, Scripting::Functions::OnCollisionEnter, other); }
	void InvokeOnCollisionStay (const Entity& other)  { InvokeOnAllScripts(&ScriptContainer::onCollisionStayFunc,  Scripting::Functions::OnCollisionStay,  other); }
	void InvokeOnCollisionExit (const Entity& other)  { InvokeOnAllScripts(&ScriptContainer::onCollisionExitFunc,  Scripting::Functions::OnCollisionExit,  other); }


	static void RegisterToInspector()
	{
		InspectorComponentRegistry::RegisterComponent<ScriptComponent>("Script", EngineAssets::Icons::LuaFile, { });
	}

private:
	// Calls one Lua callback on every script of this component (if the script defines it)
	//
	// 'callback' is a pointer to a MEMBER of ScriptContainer (e.g. &ScriptContainer::onTriggerEnterFunc)
	// It doesn't point to a function or to a specific object, it only says WHICH field to use
	//
	// 'script.*callback' then means: take that field from this specific 'script' object
	// So with callback = &ScriptContainer::onTriggerEnterFunc, 'script.*callback' is the same as 'script.onTriggerEnterFunc'
	//
	// This lets us write the loop once and reuse it for all 6 callbacks (trigger/collision enter/stay/exit)
	template<typename... Args>
	void InvokeOnAllScripts(sol::protected_function ScriptContainer::* callback, std::string_view functionName, Args&&... args)
	{
		for (auto& script : scripts)
		{
			sol::protected_function& function = script.*callback;
			if (function.valid())
				script.CallFunction(function, functionName, std::forward<Args>(args)...);
		}
	}

	static bool IsKnownCallbackName(std::string_view name)
	{
		using namespace Scripting;

		return name == std::string_view(Functions::OnStart)
			|| name == std::string_view(Functions::OnUpdate)
			|| name == std::string_view(Functions::OnTriggerEnter)
			|| name == std::string_view(Functions::OnTriggerStay)
			|| name == std::string_view(Functions::OnTriggerExit)
			|| name == std::string_view(Functions::OnCollisionEnter)
			|| name == std::string_view(Functions::OnCollisionStay)
			|| name == std::string_view(Functions::OnCollisionExit);
	}

	// A misspelled callback (e.g. 'OnUpdte') is valid Lua, so nothing would fail and nothing would happen
	// Every function a script defines that starts with "On" but isn't a known callback gets a warning
	static void WarnAboutUnknownCallbacks(const sol::environment& scriptEnvironment, const std::string& scriptPath)
	{
		scriptEnvironment.for_each([&](const sol::object& key, const sol::object& value)
			{
				if (!key.is<std::string>() || !value.is<sol::function>())
					return;

				std::string name = key.as<std::string>();
				if (name.rfind("On", 0) == 0 && !IsKnownCallbackName(name))
					SUNTA_ENGINE_LOG_WARNING("Script '{0}' defines '{1}', but the engine has no such callback (typo?)", scriptPath, name);
			});
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
	// Kinematic body gets its velocity from the distance it travelled with MovePosition,
	// so it can push other bodies it hits (setting transform.position directly is a teleport and gives no velocity)

	bool isKinematic = false;
	bool useGravity  = true;

	glm::vec3 velocity        = glm::vec3(0.0f);

	// Angular Velocity         = how the object spins
	// Direction of this vector = the axis it spins around
	// Length of this vector    = spin speed in radians / second
	// E.g. (0, 3.14, 0)		= spinning around the up-axis at half a turn per secound
	glm::vec3 angularVelocity = glm::vec3(0.0f);

	// Freeze Rotation = the body can't spin around the chosen axis (e.g. a player that must not fall over)
	// For an upright player freeze X and Z, and leave Y free so it can still turn left and right
	// The axes are the body's OWN (local) axes, the same as in Unity
	bool freezeRotationX = false;
	bool freezeRotationY = false;
	bool freezeRotationZ = false;

	// restitution = how bouncy object is when it hits something
	float mass = 1.0f;
	float restitution = 0.2f; // 0 = no bounce (sandbag), 1 = SUPER BOUNCY (rubber ball)

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

	// Only for kinematic bodies: where a script asked the body to move to (see PhysicsSystem::MovePosition)
	// Physics moves the body there during the next update and works out its velocity from the distance
	// travelled, so the body can push other bodies on the way
	glm::vec3 kinematicTarget = glm::vec3(0.0f);
	bool hasKinematicTarget = false;

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
				ADD_PROPERTY(PhysicsBodyComponent, freezeRotationX,	 PropertyDataType::Bool),
				ADD_PROPERTY(PhysicsBodyComponent, freezeRotationY,	 PropertyDataType::Bool),
				ADD_PROPERTY(PhysicsBodyComponent, freezeRotationZ,	 PropertyDataType::Bool),
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