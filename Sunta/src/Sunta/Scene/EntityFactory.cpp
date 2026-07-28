#include "Core/SuntaPreCompiled.h"
#include "EntityFactory.h"

#include "Scene.h"

#include "ECS/Component.h"
#include "Renderer/Material.h"
#include "Renderer/Mesh.h"
#include "Renderer/Primitives.h"
#include "Core/ResourceManager.h"

namespace Sunta
{

unsigned int EntityFactory::CreateCube(Scene& scene, RendererDevice& rendererDevice, const std::string& name, const glm::vec3& position)
{
	auto& entityManager = scene.GetEntityManager();

	auto cubeShader = ResourceManager::GetShaderData("Lit");
	auto cubeMaterial = std::make_shared<Material>(cubeShader);
	cubeMaterial->SetAmbient(glm::vec3(0.25f, 0.2f, 0.05f))
		.SetDiffuse(glm::vec3(0.75f, 0.6f, 0.24f))
		.SetSpecular(glm::vec3(0.63, 0.56f, 0.37f))
		.SetShininess(128.0f);

	unsigned int cubeEntity = entityManager.CreateEntity();
	entityManager.AddComponent<TagComponent>(cubeEntity).name = name;
	entityManager.AddComponent<TransformComponent>(cubeEntity, position);
	entityManager.AddComponent<WorldMatrixComponent>(cubeEntity);
	entityManager.AddComponent<MeshComponent>(cubeEntity, Primitives::CreateCube(rendererDevice), cubeMaterial);

	return cubeEntity;
}

unsigned int EntityFactory::CreatePyramid(Scene& scene, RendererDevice& rendererDevice, const std::string& name, const glm::vec3& position)
{
	auto& entityManager = scene.GetEntityManager();

	auto cubeShader = ResourceManager::GetShaderData("Lit");
	auto cubeMaterial = std::make_shared<Material>(cubeShader);
	cubeMaterial->SetAmbient(glm::vec3(0.25f, 0.2f, 0.05f))
		.SetDiffuse(glm::vec3(0.75f, 0.6f, 0.24f))
		.SetSpecular(glm::vec3(0.63, 0.56f, 0.37f))
		.SetShininess(128.0f);

	unsigned int cubeEntity = entityManager.CreateEntity();
	entityManager.AddComponent<TagComponent>(cubeEntity).name = name;
	entityManager.AddComponent<TransformComponent>(cubeEntity, position);
	entityManager.AddComponent<WorldMatrixComponent>(cubeEntity);
	entityManager.AddComponent<MeshComponent>(cubeEntity, Primitives::CreatePyramide(rendererDevice), cubeMaterial);

	return cubeEntity;
}

unsigned int EntityFactory::CreateCone(Scene& scene, RendererDevice& rendererDevice, const std::string& name, const glm::vec3& position)
{
	auto& entityManager = scene.GetEntityManager();

	auto shader = ResourceManager::GetShaderData("Lit");
	auto material = std::make_shared<Material>(shader);
	material->SetAmbient(glm::vec3(0.25f, 0.2f, 0.05f))
		.SetDiffuse(glm::vec3(0.75f, 0.6f, 0.24f))
		.SetSpecular(glm::vec3(0.63, 0.56f, 0.37f))
		.SetShininess(128.0f);

	unsigned int entity = entityManager.CreateEntity();
	entityManager.AddComponent<TagComponent>(entity).name = name;
	entityManager.AddComponent<TransformComponent>(entity, position);
	entityManager.AddComponent<WorldMatrixComponent>(entity);
	entityManager.AddComponent<MeshComponent>(entity, Primitives::CreateCone(rendererDevice), material);

	return entity;
}

unsigned int EntityFactory::CreateSphere(Scene& scene, RendererDevice& rendererDevice, const std::string& name, const glm::vec3& position)
{
	auto& entityManager = scene.GetEntityManager();

	auto sphereShader = ResourceManager::GetShaderData("Lit");
	auto sphereMaterial = std::make_shared<Material>(sphereShader);
	sphereMaterial->SetAmbient(glm::vec3(0.25f, 0.2f, 0.05f))
		.SetDiffuse(glm::vec3(0.75f, 0.6f, 0.24f))
		.SetSpecular(glm::vec3(0.63, 0.56f, 0.37f))
		.SetShininess(128.0f);

	unsigned int sphereEntity = entityManager.CreateEntity();
	entityManager.AddComponent<TagComponent>(sphereEntity).name = name;
	entityManager.AddComponent<TransformComponent>(sphereEntity, position);
	entityManager.AddComponent<WorldMatrixComponent>(sphereEntity);
	entityManager.AddComponent<MeshComponent>(sphereEntity, Primitives::CreateSphere(rendererDevice), sphereMaterial);

	return sphereEntity;
}

unsigned int EntityFactory::CreateDirectionalLight(Scene& scene, RendererDevice& rendererDevice, const std::string& name, const glm::vec3& position)
{
	auto& entityManager = scene.GetEntityManager();

	auto lightShader = ResourceManager::GetShaderData("Unlit");
	auto lightMaterial = std::make_shared<Material>(lightShader);

	unsigned int lightEntity = entityManager.CreateEntity();
	entityManager.AddComponent<TagComponent>(lightEntity).name = name;
	entityManager.AddComponent<TransformComponent>(lightEntity, position);
	entityManager.AddComponent<WorldMatrixComponent>(lightEntity);
	entityManager.AddComponent<MeshComponent>(lightEntity, Primitives::CreateCube(rendererDevice), lightMaterial);

	entityManager.AddComponent<DirectionalLightComponent>(lightEntity);

	return lightEntity;
}

unsigned int EntityFactory::CreatePointLight(Scene& scene, RendererDevice& rendererDevice, const std::string& name, const glm::vec3& position)
{
	auto& entityManager = scene.GetEntityManager();

	auto lightShader = ResourceManager::GetShaderData("Unlit");
	auto lightMaterial = std::make_shared<Material>(lightShader);

	unsigned int lightEntity = entityManager.CreateEntity();
	entityManager.AddComponent<TagComponent>(lightEntity).name = name;
	entityManager.AddComponent<TransformComponent>(lightEntity, position);
	entityManager.AddComponent<WorldMatrixComponent>(lightEntity);
	entityManager.AddComponent<MeshComponent>(lightEntity, Primitives::CreateCube(rendererDevice), lightMaterial);

	entityManager.AddComponent<PointLightComponent>(lightEntity);

	return lightEntity;
}

unsigned int EntityFactory::CreateSpotLight(Scene& scene, RendererDevice& rendererDevice, const std::string& name, const glm::vec3& position)
{
	auto& entityManager = scene.GetEntityManager();

	auto lightShader = ResourceManager::GetShaderData("Unlit");
	auto lightMaterial = std::make_shared<Material>(lightShader);

	unsigned int lightEntity = entityManager.CreateEntity();
	entityManager.AddComponent<TagComponent>(lightEntity).name = name;
	entityManager.AddComponent<TransformComponent>(lightEntity, position);
	entityManager.AddComponent<WorldMatrixComponent>(lightEntity);
	entityManager.AddComponent<MeshComponent>(lightEntity, Primitives::CreateCube(rendererDevice), lightMaterial);

	entityManager.AddComponent<SpotlightComponent>(lightEntity);

	return lightEntity;
}

}