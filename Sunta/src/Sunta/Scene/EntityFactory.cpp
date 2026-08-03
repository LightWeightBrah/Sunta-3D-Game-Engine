#include "Core/SuntaPreCompiled.h"
#include "EntityFactory.h"

#include "Scene.h"

#include "ECS/Component.h"
#include "Renderer/Material.h"
#include "Renderer/Mesh.h"
#include "Renderer/Primitives.h"
#include "Core/ResourceManager.h"
#include "Core/EngineAssets.h"
#include "Renderer/Shader.h"

namespace Sunta
{

unsigned int EntityFactory::CreateEmpty(Scene& scene, const glm::vec3& position, const std::string& name)
{
	auto& entityManager = scene.GetEntityManager();

	unsigned int entity = entityManager.CreateEntity();
	entityManager.AddComponent<TagComponent>(entity).name = name;
	entityManager.AddComponent<TransformComponent>(entity, position);
	entityManager.AddComponent<WorldMatrixComponent>(entity);

	return entity;
}

static std::shared_ptr<Material> GetDefaultMaterial()
{
	auto shader = ResourceManager::GetShaderData(Sunta::EngineAssets::Shaders::Lit);

	auto material = std::make_shared<Material>(shader);
	material->SetAmbient(glm::vec3(0.25f, 0.2f, 0.05f))
		.SetDiffuse(glm::vec3(0.75f, 0.6f, 0.24f))
		.SetSpecular(glm::vec3(0.63, 0.56f, 0.37f))
		.SetShininess(128.0f);

	return material;
}

unsigned int EntityFactory::CreateCube(Scene& scene, RendererDevice& rendererDevice, const glm::vec3& position, const std::string& name, std::shared_ptr<Material> customMaterial)
{
	unsigned int entity = CreateEmpty(scene, position, name);
	auto material = customMaterial ? customMaterial : GetDefaultMaterial();

	scene.GetEntityManager().AddComponent<MeshComponent>(entity, Primitives::CreateCube(rendererDevice), material);
	return entity;
}

unsigned int EntityFactory::CreatePyramid(Scene& scene, RendererDevice& rendererDevice, const glm::vec3& position, const std::string& name, std::shared_ptr<Material> customMaterial)
{
	unsigned int entity = CreateEmpty(scene, position, name);
	auto material = customMaterial ? customMaterial : GetDefaultMaterial();

	scene.GetEntityManager().AddComponent<MeshComponent>(entity, Primitives::CreatePyramide(rendererDevice), material);
	return entity;
}

unsigned int EntityFactory::CreateCone(Scene& scene, RendererDevice& rendererDevice, const glm::vec3& position, const std::string& name, std::shared_ptr<Material> customMaterial)
{
	unsigned int entity = CreateEmpty(scene, position, name);
	auto material = customMaterial ? customMaterial : GetDefaultMaterial();

	scene.GetEntityManager().AddComponent<MeshComponent>(entity, Primitives::CreateCone(rendererDevice), material);
	return entity;
}

unsigned int EntityFactory::CreateSphere(Scene& scene, RendererDevice& rendererDevice, const glm::vec3& position, const std::string& name, std::shared_ptr<Material> customMaterial)
{
	unsigned int entity = CreateEmpty(scene, position, name);
	auto material = customMaterial ? customMaterial : GetDefaultMaterial();

	scene.GetEntityManager().AddComponent<MeshComponent>(entity, Primitives::CreateSphere(rendererDevice), material);
	return entity;
}

unsigned int EntityFactory::CreateCapsule(Scene& scene, RendererDevice& rendererDevice, const glm::vec3& position, const std::string& name, std::shared_ptr<Material> customMaterial)
{
	unsigned int entity = CreateEmpty(scene, position, name);
	auto material = customMaterial ? customMaterial : GetDefaultMaterial();

	scene.GetEntityManager().AddComponent<MeshComponent>(entity, Primitives::CreateCapsule(rendererDevice), material);
	return entity;
}

unsigned int EntityFactory::CreateDirectionalLight(Scene& scene, RendererDevice& rendererDevice, const glm::vec3& position, const std::string& name, std::shared_ptr<Material> customMaterial)
{
	unsigned int entity = CreateEmpty(scene, position, name);
	auto material = customMaterial ? customMaterial : std::make_shared<Material>(ResourceManager::GetShaderData(EngineAssets::Shaders::Unlit));

	auto& entityManager = scene.GetEntityManager();
	entityManager.AddComponent<MeshComponent>(entity, Primitives::CreateCube(rendererDevice), material);
	entityManager.AddComponent<DirectionalLightComponent>(entity);
	return entity;
}

unsigned int EntityFactory::CreatePointLight(Scene& scene, RendererDevice& rendererDevice, const glm::vec3& position, const std::string& name, std::shared_ptr<Material> customMaterial)
{
	unsigned int entity = CreateEmpty(scene, position, name);
	auto material = customMaterial ? customMaterial : std::make_shared<Material>(ResourceManager::GetShaderData(EngineAssets::Shaders::Unlit));

	auto& entityManager = scene.GetEntityManager();
	entityManager.AddComponent<MeshComponent>(entity, Primitives::CreateCube(rendererDevice), material);
	entityManager.AddComponent<PointLightComponent>(entity);
	return entity;
}

unsigned int EntityFactory::CreateSpotLight(Scene& scene, RendererDevice& rendererDevice, const glm::vec3& position, const std::string& name, std::shared_ptr<Material> customMaterial)
{
	unsigned int entity = CreateEmpty(scene, position, name);
	auto material = customMaterial ? customMaterial : std::make_shared<Material>(ResourceManager::GetShaderData(EngineAssets::Shaders::Unlit));

	auto& entityManager = scene.GetEntityManager();
	entityManager.AddComponent<MeshComponent>(entity, Primitives::CreateCube(rendererDevice), material);
	entityManager.AddComponent<SpotlightComponent>(entity);
	return entity;
}

}