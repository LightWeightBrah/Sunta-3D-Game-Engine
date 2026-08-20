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
			model = glm::translate(model, transforms[i].position);

			model = glm::rotate(model, glm::radians(transforms[i].rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
			model = glm::rotate(model, glm::radians(transforms[i].rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
			model = glm::rotate(model, glm::radians(transforms[i].rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));

			model = glm::scale(model, transforms[i].scale);

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

void Systems::UpdateScripts(EntityManager& entityManager, float deltaTime)
{
	auto& scriptComponents = entityManager.GetAllComponents<ScriptComponent>();

	for (auto& component : scriptComponents)
	{
		for (auto& script : component.scripts)
		{
			if(script.scriptPath.empty())
				continue;

			if (std::filesystem::exists(script.scriptPath))
			{
				auto currentWriteTime = std::filesystem::last_write_time(script.scriptPath).time_since_epoch().count();

				if (currentWriteTime > script.lastWriteTime)
				{
					script.lastWriteTime = currentWriteTime;
					component.ReloadScript(script, component.entityID);

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

}