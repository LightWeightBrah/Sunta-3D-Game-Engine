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

static std::shared_ptr<Material> GetDefaultMaterial()
{
	return ResourceManager::GetMaterialData(Sunta::EngineAssets::Materials::Default);
}

static std::shared_ptr<Material> GetUnlitMaterial()
{
	return ResourceManager::GetMaterialData(Sunta::EngineAssets::Materials::Unlit);
}

static std::string GetMaterialName(const std::shared_ptr<Material>& material)
{
	return material ? material->GetName() : "";
}

unsigned int EntityFactory::CreateEmpty(Scene& scene, const glm::vec3& position, const std::string& name)
{
	auto& entityManager = scene.GetEntityManager();

	unsigned int entity = entityManager.CreateEntity();
	entityManager.AddComponent<TagComponent>(entity).name = name;
	entityManager.AddComponent<TransformComponent>(entity, position);
	entityManager.AddComponent<WorldMatrixComponent>(entity);

	return entity;
}

unsigned int EntityFactory::CreateCube(Scene& scene, const glm::vec3& position, const std::string& name, std::shared_ptr<Material> customMaterial)
{
	using namespace Sunta::EngineAssets;

	unsigned int entity = CreateEmpty(scene, position, name);
	auto material = customMaterial ? customMaterial : GetDefaultMaterial();
	auto mesh = ResourceManager::GetMeshData(Meshes::Cube);

	scene.GetEntityManager().AddComponent<MeshComponent>(entity, mesh, material, 
		Meshes::Cube, GetMaterialName(material));
	return entity;
}

unsigned int EntityFactory::CreatePyramid(Scene& scene, const glm::vec3& position, const std::string& name, std::shared_ptr<Material> customMaterial)
{
	using namespace Sunta::EngineAssets;

	unsigned int entity = CreateEmpty(scene, position, name);
	auto material = customMaterial ? customMaterial : GetDefaultMaterial();
	auto mesh = ResourceManager::GetMeshData(Meshes::Pyramid);

	scene.GetEntityManager().AddComponent<MeshComponent>(entity, mesh, material,
		Meshes::Pyramid, GetMaterialName(material));
	return entity;
}

unsigned int EntityFactory::CreateCone(Scene& scene, const glm::vec3& position, const std::string& name, std::shared_ptr<Material> customMaterial)
{
	using namespace Sunta::EngineAssets;
	unsigned int entity = CreateEmpty(scene, position, name);
	auto material = customMaterial ? customMaterial : GetDefaultMaterial();
	auto mesh = ResourceManager::GetMeshData(Meshes::Cone);

	scene.GetEntityManager().AddComponent<MeshComponent>(entity, mesh, material,
		Meshes::Cone, GetMaterialName(material));
	return entity;
}

unsigned int EntityFactory::CreateSphere(Scene& scene, const glm::vec3& position, const std::string& name, std::shared_ptr<Material> customMaterial)
{
	using namespace Sunta::EngineAssets;
	unsigned int entity = CreateEmpty(scene, position, name);
	auto material = customMaterial ? customMaterial : GetDefaultMaterial();
	auto mesh = ResourceManager::GetMeshData(Meshes::Sphere);

	scene.GetEntityManager().AddComponent<MeshComponent>(entity, mesh, material,
		Meshes::Sphere, GetMaterialName(material));
	return entity;
}

unsigned int EntityFactory::CreateCapsule(Scene& scene, const glm::vec3& position, const std::string& name, std::shared_ptr<Material> customMaterial)
{
	using namespace Sunta::EngineAssets;
	unsigned int entity = CreateEmpty(scene, position, name);
	auto material = customMaterial ? customMaterial : GetDefaultMaterial();
	auto mesh = ResourceManager::GetMeshData(Meshes::Capsule);

	scene.GetEntityManager().AddComponent<MeshComponent>(entity, mesh, material,
		Meshes::Capsule, GetMaterialName(material));
	return entity;
}

unsigned int EntityFactory::CreateDirectionalLight(Scene& scene, const glm::vec3& position, const std::string& name, std::shared_ptr<Material> customMaterial)
{
	using namespace Sunta::EngineAssets;
	unsigned int entity = CreateEmpty(scene, position, name);
	auto material = customMaterial ? customMaterial : GetUnlitMaterial();
	auto mesh = ResourceManager::GetMeshData(Meshes::Cube);

	auto& entityManager = scene.GetEntityManager();
	entityManager.AddComponent<MeshComponent>(entity, mesh, material, Meshes::Cube, GetMaterialName(material));
	entityManager.AddComponent<DirectionalLightComponent>(entity);
	return entity;
}

unsigned int EntityFactory::CreatePointLight(Scene& scene, const glm::vec3& position, const std::string& name, std::shared_ptr<Material> customMaterial)
{
	using namespace Sunta::EngineAssets;
	unsigned int entity = CreateEmpty(scene, position, name);
	auto material = customMaterial ? customMaterial : GetUnlitMaterial();
	auto mesh = ResourceManager::GetMeshData(Meshes::Cube);

	auto& entityManager = scene.GetEntityManager();
	entityManager.AddComponent<MeshComponent>(entity, mesh, material, Meshes::Cube, GetMaterialName(material));
	entityManager.AddComponent<PointLightComponent>(entity);
	return entity;
}

unsigned int EntityFactory::CreateSpotLight(Scene& scene, const glm::vec3& position, const std::string& name, std::shared_ptr<Material> customMaterial)
{
	using namespace Sunta::EngineAssets;
	unsigned int entity = CreateEmpty(scene, position, name);
	auto material = customMaterial ? customMaterial : GetUnlitMaterial();
	auto mesh = ResourceManager::GetMeshData(Meshes::Cube);

	auto& entityManager = scene.GetEntityManager();
	entityManager.AddComponent<MeshComponent>(entity, mesh, material, Meshes::Cube, GetMaterialName(material));
	entityManager.AddComponent<SpotlightComponent>(entity);
	return entity;
}

}