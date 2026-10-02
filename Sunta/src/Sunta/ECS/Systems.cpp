#include "Core/SuntaPreCompiled.h"
#include "Systems.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Core/ResourceManager.h"
#include "EntityManager.h"
#include "Component.h"
#include "Core/EngineAssets.h"

namespace Sunta
{

void Systems::UpdateTransform(EntityManager& entityManager)
{
	auto& transforms = entityManager.GetAllComponents<TransformComponent>();
	auto& matrices = entityManager.GetAllComponents<WorldMatrixComponent>();

	for (int i = 0; i < transforms.size(); i++)
	{
		if (transforms[i].isDirty)
		{
			glm::mat4 model = glm::mat4(1.0f);

			glm::mat4 translation = glm::translate(model, transforms[i].position);
			glm::mat4 rotation    = glm::mat4_cast(transforms[i].rotationQuaternion);
			glm::mat4 scale       = glm::scale(model, transforms[i].scale);

			model = translation * rotation * scale;

			matrices[i].matrix = model;
			transforms[i].isDirty = false;
		}

	}
}

void Systems::SyncMeshComponents(EntityManager& entityManager)
{
	auto& meshComponents = entityManager.GetAllComponents<MeshComponent>();

	using namespace Sunta::EngineAssets;

	auto errorMesh = ResourceManager::GetMeshData(Meshes::Cube);
	auto errorMaterial = ResourceManager::GetMaterialData(Materials::Error);

	for (auto& component : meshComponents)
	{
		if (component.isDirty)
		{
			std::shared_ptr<Mesh> mesh = nullptr;
			if(component.meshName != MeshComponent::NULL_ASSET_NAME)
				mesh = ResourceManager::GetMeshData(component.meshName);

			std::shared_ptr<Material> material = nullptr;
			if(component.materialName != MeshComponent::NULL_ASSET_NAME)
				material = ResourceManager::GetMaterialData(component.materialName);

			component.mesh	   = (mesh != nullptr)     ? mesh     : errorMesh;
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
		}

	}
}

void Systems::UpdateAnimators(EntityManager& entityManager, float deltaTime)
{
	auto& animatorComponents = entityManager.GetAllComponents<AnimatorComponent>();

	for (auto& component : animatorComponents)
		component.animator.UpdateAnimation(deltaTime);
}

void Systems::UpdateScripts(EntityManager& entityManager, float deltaTime)
{
	auto& scriptComponents = entityManager.GetAllComponents<ScriptComponent>();

	for (auto& component : scriptComponents)
	{
		for (auto& script : component.scripts)
		{
			if(script.scriptPath.empty())
				continue;

			unsigned int entityID = entityManager.GetEntityIDForComponent(component);

			// First update after Play: run the Lua file + OnCreate now,
			// not when the script was attached in the editor
			if (!script.isStarted)
			{
				script.isStarted = true;

				// Set before loading, so the hot-reload check below doesn't reload it again
				if (std::filesystem::exists(script.scriptPath))
					script.lastWriteTime = std::filesystem::last_write_time(script.scriptPath).time_since_epoch().count();

				component.ReloadScript(script, entityID);
			}

			if (std::filesystem::exists(script.scriptPath))
			{
				auto currentWriteTime = std::filesystem::last_write_time(script.scriptPath).time_since_epoch().count();

				if (currentWriteTime > script.lastWriteTime)
				{
					script.lastWriteTime = currentWriteTime;

					component.ReloadScript(script, entityID);

					SUNTA_ENGINE_LOG_INFO("Hot-Reloaded script: '{0}'", script.scriptPath);
				}
			}

			if (script.onUpdateFunc)
			{
				script.onUpdateFunc(deltaTime);
			}
		}
	}
}

void Systems::DispatchTriggerEnter(EntityManager& entityManager, unsigned int triggerEntityID, unsigned int otherEntityID)
{
	if (auto* triggerScript = entityManager.GetComponent<ScriptComponent>(triggerEntityID))
		triggerScript->InvokeOnTriggerEnter(otherEntityID);

	if (auto* otherScript = entityManager.GetComponent<ScriptComponent>(otherEntityID))
		otherScript->InvokeOnTriggerEnter(triggerEntityID);
}

void Systems::DispatchTriggerStay(EntityManager& entityManager, unsigned int triggerEntityID, unsigned int otherEntityID)
{
	if (auto* triggerScript = entityManager.GetComponent<ScriptComponent>(triggerEntityID))
		triggerScript->InvokeOnTriggerStay(otherEntityID);

	if (auto* otherScript = entityManager.GetComponent<ScriptComponent>(otherEntityID))
		otherScript->InvokeOnTriggerStay(triggerEntityID);
}

void Systems::DispatchTriggerExit(EntityManager& entityManager, unsigned int triggerEntityID, unsigned int otherEntityID)
{
	if (auto* triggerScript = entityManager.GetComponent<ScriptComponent>(triggerEntityID))
		triggerScript->InvokeOnTriggerExit(otherEntityID);

	if (auto* otherScript = entityManager.GetComponent<ScriptComponent>(otherEntityID))
		otherScript->InvokeOnTriggerExit(triggerEntityID);
}

void Systems::DispatchCollisionEnter(EntityManager& entityManager, unsigned int collisionEntityID, unsigned int otherEntityID)
{
	if (auto* scriptA = entityManager.GetComponent<ScriptComponent>(collisionEntityID))
		scriptA->InvokeOnCollisionEnter(otherEntityID);

	if (auto* scriptB = entityManager.GetComponent<ScriptComponent>(otherEntityID))
		scriptB->InvokeOnCollisionEnter(collisionEntityID);
}

void Systems::DispatchCollisionStay(EntityManager& entityManager, unsigned int collisionEntityID, unsigned int otherEntityID)
{

	if (auto* scriptA = entityManager.GetComponent<ScriptComponent>(collisionEntityID))
		scriptA->InvokeOnCollisionStay(otherEntityID);

	if (auto* scriptB = entityManager.GetComponent<ScriptComponent>(otherEntityID))
		scriptB->InvokeOnCollisionStay(collisionEntityID);
}

void Systems::DispatchCollisionExit(EntityManager& entityManager, unsigned int collisionEntityID, unsigned int otherEntityID)
{
	if (auto* scriptA = entityManager.GetComponent<ScriptComponent>(collisionEntityID))
		scriptA->InvokeOnCollisionExit(otherEntityID);

	if (auto* scriptB = entityManager.GetComponent<ScriptComponent>(otherEntityID))
		scriptB->InvokeOnCollisionExit(collisionEntityID);
}

}