#include "Core/SuntaPreCompiled.h"
#include "Systems.h"
#include "Gameplay/CharacterActionsSystem.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Core/ResourceManager.h"
#include "EntityManager.h"
#include "Component.h"
#include "Core/EngineAssets.h"
#include "Scene/EntityHierarchy.h"
#include "Gameplay/CombatSystem.h"

namespace Sunta
{

// Every frame for every entity (not only the changed ones): a child moves whenever its parent or the parent's bone does
void Systems::UpdateTransform(EntityManager& entityManager)
{
	EntityHierarchy::UpdateWorldMatrices(entityManager);
}

void Systems::SyncMeshComponents(EntityManager& entityManager)
{
	auto& meshComponents = entityManager.GetAllComponents<MeshComponent>();

	using namespace Sunta::EngineAssets;

	for (auto& component : meshComponents)
	{
		if (component.isDirty)
		{
			// Fallbacks are looked up only when a component needs them, so a scene without meshes touches no resources
			auto errorMesh     = ResourceManager::GetMeshData(Meshes::Cube);
			auto errorMaterial = ResourceManager::GetMaterialData(Materials::Error);

			std::shared_ptr<Mesh> mesh = nullptr;
			if (component.meshName != MeshComponent::NULL_ASSET_NAME)
				mesh = ResourceManager::GetMeshData(component.meshName);

			std::shared_ptr<Material> material = nullptr;
			if (component.materialName != MeshComponent::NULL_ASSET_NAME)
				material = ResourceManager::GetMaterialData(component.materialName);

			component.mesh = (mesh != nullptr) ? mesh : errorMesh;
			component.material = (material != nullptr) ? material : errorMaterial;

			component.isDirty = false;
		}
	}
}

void Systems::SyncModelComponents(EntityManager& entityManager)
{
	auto& modelComponents = entityManager.GetAllComponents<ModelComponent>();

	for (auto& component : modelComponents)
	{
		unsigned int entityID = entityManager.GetEntityIDForComponent(component);
		auto* animatorComponent = entityManager.GetComponent<AnimatorComponent>(entityID);
		bool needsAnimatorSync = animatorComponent && animatorComponent->needsModelSync;

		if (!component.isDirty && !needsAnimatorSync)
			continue;

		const bool isSwapped = component.isDirty && component.modelData != nullptr;
		if (component.isDirty)
		{
			component.modelData = (!component.modelName.empty() && component.modelName != ModelComponent::NULL_ASSET_NAME)
				? ResourceManager::GetModelData(component.modelName)
				: nullptr;

			component.isDirty = false;
		}

		if (component.modelData && component.modelData->model && animatorComponent)
		{
			animatorComponent->animator = Animator(component.modelData->model.get());

			auto& animations = component.modelData->animations;

			if (!animations.empty())
			{
				bool hasValidSelection = !animatorComponent->currentAnimationName.empty()
					&& animations.find(animatorComponent->currentAnimationName) != animations.end();

				if (!hasValidSelection)
					animatorComponent->currentAnimationName = animations.begin()->first;

				animatorComponent->animator.PlayAnimation(&animations.at(animatorComponent->currentAnimationName));

				// Bone matrices are only filled by UpdateAnimation (runs in Play)
				// Sample frame 0 once, so the model isn't drawn with identity matrices in the editor
				animatorComponent->animator.SampleAtTime(0.0f);
			}
			else
			{
				animatorComponent->currentAnimationName.clear();
			}

			animatorComponent->needsModelSync = false;

			// A model put on while the game plays (a boss changing his body): what the character was doing goes on
			if (isSwapped)
				CharacterActionsSystem::RestartAction(entityManager, entityID);
		}

	}
}

void Systems::UpdateAnimators(EntityManager& entityManager, float deltaTime)
{
	auto& animatorComponents = entityManager.GetAllComponents<AnimatorComponent>();

	for (auto& component : animatorComponents)
	{
		if (!component.springBones.prefix.empty())
		{
			if (!component.areSpringsSet)
			{
				component.animator.SetSpringBones(component.springBones);
				component.areSpringsSet = true;
			}
			const auto entity = entityManager.GetEntityIDForComponent(component);
			if (const auto* world = entityManager.GetComponent<WorldMatrixComponent>(entity))
				component.animator.SetWorldTransform(world->matrix);
		}
		component.animator.UpdateAnimation(deltaTime);
	}
}

namespace
{

const char* GetDamageKindName(DamageKind kind)
{
	switch (kind)
	{
	case DamageKind::Weapon: return "weapon";
	case DamageKind::Spell:  return "spell";
	default:                 return "other";
	}
}

// Everyone hurt since the scripts ran the last time hears of it now (OnDamaged)
void TellScriptsAboutDamage(EntityManager& entityManager)
{
	for (const DamageEvent& event : CombatSystem::TakeDamageEvents())
	{
		ScriptComponent* script = entityManager.GetComponent<ScriptComponent>(event.targetEntityID);
		if (script)
			script->InvokeOnDamaged(event.damage, GetDamageKindName(event.kind), Entity(event.attackerEntityID, &entityManager));
	}
}

}

void Systems::UpdateScripts(EntityManager& entityManager, float deltaTime, bool isHotReloadEnabled)
{
	// The owners as they are now: a script may spawn entities with scripts of their own (they start next frame), which
	// moves the components in memory - so each one is looked up again before each of its scripts
	std::vector<unsigned int> owners;
	for (auto& component : entityManager.GetAllComponents<ScriptComponent>())
		owners.push_back(entityManager.GetEntityIDForComponent(component));

	for (unsigned int ownerID : owners)
	{
		Entity owner(ownerID, &entityManager);

		for (size_t scriptIndex = 0; ; scriptIndex++)
		{
			ScriptComponent* found = entityManager.GetComponent<ScriptComponent>(ownerID);
			if (!found || scriptIndex >= found->scripts.size())
				break;
			ScriptComponent& component = *found;
			auto& script = component.scripts[scriptIndex];

			if (script.scriptPath.empty())
				continue;

			// First update after Play: run the Lua file + OnCreate now,
			// not when the script was attached in the editor
			if (!script.isStarted)
			{
				script.isStarted = true;

				// Set before loading, so the hot-reload check below doesn't reload it again
				if (std::filesystem::exists(script.scriptPath))
					script.lastWriteTime = std::filesystem::last_write_time(script.scriptPath).time_since_epoch().count();

				component.ReloadScript(script, owner);
			}

			if (isHotReloadEnabled && std::filesystem::exists(script.scriptPath))
			{
				auto currentWriteTime = std::filesystem::last_write_time(script.scriptPath).time_since_epoch().count();

				if (currentWriteTime > script.lastWriteTime)
				{
					script.lastWriteTime = currentWriteTime;

					component.ReloadScript(script, owner);

					SUNTA_ENGINE_LOG_INFO("Hot-Reloaded script: '{0}'", script.scriptPath);
				}
			}

			if (script.onUpdateFunc.valid())
				script.CallFunction(script.onUpdateFunc, Scripting::Functions::OnUpdate, deltaTime);
		}
	}

	TellScriptsAboutDamage(entityManager);
}

namespace
{

// Runs 'notify' on the script of entity A and B (if they have one)
// each time passing the OTHER entity as the argument
template<typename NotifyFunction>
void NotifyBothScripts(EntityManager& entityManager, unsigned int firstID, unsigned int secondID, NotifyFunction notify)
{
	Entity first(firstID, &entityManager);
	Entity second(secondID, &entityManager);

	if (auto* firstScript = first.GetComponent<ScriptComponent>())
		notify(*firstScript, second);

	if (auto* secondScript = second.GetComponent<ScriptComponent>())
		notify(*secondScript, first);
}

}

void Systems::DispatchTriggerEnter(EntityManager& entityManager, unsigned int triggerEntityID, unsigned int otherEntityID)
{
	NotifyBothScripts(entityManager, triggerEntityID, otherEntityID,
		[](ScriptComponent& script, const Entity& other) { script.InvokeOnTriggerEnter(other); });
}

void Systems::DispatchTriggerStay(EntityManager& entityManager, unsigned int triggerEntityID, unsigned int otherEntityID)
{
	NotifyBothScripts(entityManager, triggerEntityID, otherEntityID,
		[](ScriptComponent& script, const Entity& other) { script.InvokeOnTriggerStay(other); });
}

void Systems::DispatchTriggerExit(EntityManager& entityManager, unsigned int triggerEntityID, unsigned int otherEntityID)
{
	NotifyBothScripts(entityManager, triggerEntityID, otherEntityID,
		[](ScriptComponent& script, const Entity& other) { script.InvokeOnTriggerExit(other); });
}

void Systems::DispatchCollisionEnter(EntityManager& entityManager, unsigned int collisionEntityID, unsigned int otherEntityID)
{
	NotifyBothScripts(entityManager, collisionEntityID, otherEntityID,
		[](ScriptComponent& script, const Entity& other) { script.InvokeOnCollisionEnter(other); });
}

void Systems::DispatchCollisionStay(EntityManager& entityManager, unsigned int collisionEntityID, unsigned int otherEntityID)
{
	NotifyBothScripts(entityManager, collisionEntityID, otherEntityID,
		[](ScriptComponent& script, const Entity& other) { script.InvokeOnCollisionStay(other); });
}

void Systems::DispatchCollisionExit(EntityManager& entityManager, unsigned int collisionEntityID, unsigned int otherEntityID)
{
	NotifyBothScripts(entityManager, collisionEntityID, otherEntityID,
		[](ScriptComponent& script, const Entity& other) { script.InvokeOnCollisionExit(other); });
}

}