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

unsigned int EntityFactory::CreateCube(Scene& scene, RendererDevice& rendererDevice, const std::string& name /*= "Cube"*/, const glm::vec3& position /*= glm::vec3(0.0f)*/)
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

//TODO: Add light creation
unsigned int EntityFactory::CreateDirectionalLight(Scene& scene, RendererDevice& rendererDevice, const std::string& name /*= "Directional Light"*/, const glm::vec3& position /*= glm::vec3(0.0f)*/)
{
	return 0;
}

unsigned int EntityFactory::CreatePointLight(Scene& scene, RendererDevice& rendererDevice, const std::string& name /*= "Point Light"*/, const glm::vec3& position /*= glm::vec3(0.0f)*/)
{
	return 0;
}

unsigned int EntityFactory::CreateSpotLight(Scene& scene, RendererDevice& rendererDevice, const std::string& name /*= "Spot Light"*/, const glm::vec3& position /*= glm::vec3(0.0f)*/)
{
	return 0;
}

}