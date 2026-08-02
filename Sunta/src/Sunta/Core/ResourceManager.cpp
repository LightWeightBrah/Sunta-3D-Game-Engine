#include "SuntaPreCompiled.h"
#include "ResourceManager.h"

#include "Log.h"
#include "Renderer/Texture.h"
#include "Renderer/Shader.h"
#include "Renderer/Material.h"
#include "Renderer/Model.h"
#include "Animation/Animation.h"
#include "Renderer/RendererDevice.h"

namespace Sunta
{

void ResourceManager::LoadEditorIcon(const std::string& name, const std::string& filepath)
{
	if (editorIconsRegistered.count(name))
	{
		SUNTA_ENGINE_LOG_ERROR("Resource Manager: Couldn't find editor icon named: '{0}' filepath: '{1}'", name, filepath);
		return;
	}

	if (!rendererDevice)
	{
		SUNTA_ENGINE_LOG_ERROR("Resource Manager: Renderer device is NULL! You should call Renderer.Init() first");
		return;
	}

	auto iconTexture = rendererDevice->CreateTexture(filepath, false);
	editorIconsRegistered[name] = iconTexture;

	SUNTA_ENGINE_LOG_INFO("Resource Manager: Registered Editor Icon: '{0}' filepath: '{1}'", name, filepath);
}

void ResourceManager::LoadModel(const std::string& name, const std::string& filepath)
{
	if (modelsRegistered.count(name))
	{
		SUNTA_ENGINE_LOG_ERROR("Resource Manager: Couldn't find model named: '{0}' filepath: '{1}'", name, filepath);
		return;
	}

	if (!rendererDevice)
	{
		SUNTA_ENGINE_LOG_ERROR("Resource Manager: Renderer device is NULL! You should call Renderer.Init() first");
		return;
	}

	auto data = std::make_shared<ModelData>();

	data->model = std::make_shared<Model>(*rendererDevice, filepath, false);

	data->animations[AnimationType::IDLE]    = Animation(filepath, data->model.get(), 0);
	data->animations[AnimationType::GESTURE] = Animation(filepath, data->model.get(), 1);
	data->animations[AnimationType::RUNNING] = Animation(filepath, data->model.get(), 2);

	modelsRegistered[name] = data;
	SUNTA_ENGINE_LOG_INFO("Resource Manager: Registered Model: '{0}' filepath: '{1}'", name, filepath);
}

void ResourceManager::LoadMesh(const std::string& name, std::function<std::shared_ptr<Mesh>()> primitiveFactory)
{
	if (meshesRegistered.count(name))
	{
		SUNTA_ENGINE_LOG_ERROR("Resource Manager: Couldn't find Primitive Mesh named: '{0}'", name);
		return;
	}

	auto mesh = primitiveFactory();
	if (mesh)
	{
		meshesRegistered[name] = mesh;
		SUNTA_ENGINE_LOG_INFO("Resource Manager: Registered Primitive Mesh: '{0}'", name);
	}
}

void ResourceManager::LoadMaterial(const std::string& name, std::shared_ptr<Material> material)
{
	if (materialsRegistered.count(name))
	{
		SUNTA_ENGINE_LOG_ERROR("Resource Manager: Couldn't find Material named: '{0}'", name);
		return;
	}

	materialsRegistered[name] = material;

	SUNTA_ENGINE_LOG_INFO("Resource Manager: Registered Material: '{0}'", name);
}

void ResourceManager::LoadTexture(const std::string& name, const std::string& filepath)
{
	if (texturesRegistered.count(name))
	{
		SUNTA_ENGINE_LOG_ERROR("Resource Manager: Couldn't find texture named: '{0}' filepath: '{1}'", name, filepath);
		return;
	}

	if (!rendererDevice)
	{
		SUNTA_ENGINE_LOG_ERROR("Resource Manager: Renderer device is NULL! You should call Renderer.Init() first");
		return;
	}

	auto texture = rendererDevice->CreateTexture(filepath);
	texturesRegistered[name] = texture;

	SUNTA_ENGINE_LOG_INFO("Resource Manager: Registered Texture: '{0}' filepath: '{1}'", name, filepath);
}

void ResourceManager::LoadShader(const std::string& name, const std::string& filepath)
{
	if (shadersRegistered.count(name))
	{
		SUNTA_ENGINE_LOG_ERROR("Resource Manager: Couldn't find shader named: '{0}' filepath: '{1}'", name, filepath);
		return;
	}

	if (!rendererDevice)
	{
		SUNTA_ENGINE_LOG_ERROR("Resource Manager: Renderer device is NULL! You should call Renderer.Init() first");
		return;
	}

	auto shader = rendererDevice->CreateShader(filepath);
	shadersRegistered[name] = shader;

	SUNTA_ENGINE_LOG_INFO("Resource Manager: Registered Shader: '{0}' filepath: '{1}'", name, filepath);
}

std::shared_ptr<Texture> ResourceManager::GetEditorIcon(const std::string& name)
{
	auto it = editorIconsRegistered.find(name);
	if (it == editorIconsRegistered.end())
	{
		SUNTA_ENGINE_LOG_ERROR("Resource Manager::GetEditorIcon: Couldn't find Editor Icon named: '{0}'", name);
		return nullptr;
	}
	return it->second;
}

std::shared_ptr<ModelData> ResourceManager::GetModelData(const std::string& name)
{
	auto it = modelsRegistered.find(name);
	if (it == modelsRegistered.end())
	{
		SUNTA_ENGINE_LOG_ERROR("Resource Manager::GetModelData: Couldn't find Model named: '{0}'", name);
		return nullptr;
	}
	return it->second;
}

std::shared_ptr<Mesh> ResourceManager::GetMeshData(const std::string& name)
{
	auto it = meshesRegistered.find(name);
	if (it == meshesRegistered.end())
	{
		SUNTA_ENGINE_LOG_ERROR("Resource Manager::GetMeshData: Couldn't find Mesh named: '{0}'", name);
		return nullptr;
	}
	return it->second;
}

std::shared_ptr<Texture> ResourceManager::GetTextureData(const std::string& name)
{
	auto it = texturesRegistered.find(name);
	if (it == texturesRegistered.end())
	{
		SUNTA_ENGINE_LOG_ERROR("Resource Manager::GetTextureData: Couldn't find Texture named: '{0}'", name);
		return nullptr;
	}
	return it->second;
}

std::shared_ptr<Shader> ResourceManager::GetShaderData(const std::string& name)
{
	auto it = shadersRegistered.find(name);
	if (it == shadersRegistered.end())
	{
		SUNTA_ENGINE_LOG_ERROR("Resource Manager::GetShaderData: Couldn't find Shader named: '{0}'", name);
		return nullptr;
	}
	return it->second;
}

std::shared_ptr<Material> ResourceManager::GetMaterialData(const std::string& name)
{
	auto it = materialsRegistered.find(name);
	if (it == materialsRegistered.end())
	{
		SUNTA_ENGINE_LOG_ERROR("Resource Manager::GetMaterialData: Couldn't find Material named: '{0}'", name);
		return nullptr;
	}
	return it->second;
}

std::shared_ptr<Material> ResourceManager::LoadOrGetModelMaterial(const std::string& name, std::shared_ptr<Shader> shader)
{
	auto it = materialsRegistered.find(name);
	if (it == materialsRegistered.end())
	{
		auto newMaterial = std::make_shared<Material>(shader);
		materialsRegistered[name] = newMaterial;

		SUNTA_ENGINE_LOG_INFO("ResourceManager::LoadOrGetModelMaterial Loaded new Material '{0}'", name);
		return newMaterial;
	}

	SUNTA_ENGINE_LOG_INFO("ResourceManager::LoadOrGetModelMaterial Loaded existing Material '{0}'", name);
	return it->second;
}

std::shared_ptr<Texture> ResourceManager::LoadOrGetModelTexture(const std::string& filepath)
{
	auto it = texturesRegistered.find(filepath);
	if (it == texturesRegistered.end())
	{
		auto newTexture = rendererDevice->CreateTexture(filepath);
		texturesRegistered[filepath] = newTexture;

		SUNTA_ENGINE_LOG_INFO("ResourceManager::LoadOrGetModelTexture Loaded new Texture '{0}'", filepath);
		return newTexture;
	}


	SUNTA_ENGINE_LOG_INFO("ResourceManager::LoadOrGetModelTexture Loaded existing Texture '{0}'", filepath);
	return it->second;
}

std::vector<std::string> ResourceManager::GetMeshesNames()
{
	std::vector<std::string> names;
	for (const auto& [name, mesh] : meshesRegistered)
		names.push_back(name);

	return names;

}

std::vector<std::string> ResourceManager::GetMaterialsNames()
{
	std::vector<std::string> names;
	for (const auto& [name, material] : materialsRegistered)
		names.push_back(name);

	return names;
}

}