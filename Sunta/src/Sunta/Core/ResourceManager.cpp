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

void ResourceManager::LoadModel(const std::string& name, const std::string& path)
{
	if (modelsRegistered.count(name))
	{
		SUNTA_ENGINE_LOG_ERROR("Resource Manager: Couldn't find model named: {}", name);
		return;
	}

	if (!rendererDevice)
	{
		SUNTA_ENGINE_LOG_ERROR("Resource Manager: Renderer device is NULL! You should call Renderer.Init() first");
		return;
	}

	auto data = std::make_shared<ModelData>();

	data->model = std::make_shared<Model>(*rendererDevice, path, false);

	data->animations[AnimationType::IDLE]    = Animation(path, data->model.get(), 0);
	data->animations[AnimationType::GESTURE] = Animation(path, data->model.get(), 1);
	data->animations[AnimationType::RUNNING] = Animation(path, data->model.get(), 2);

	modelsRegistered[name] = data;
	SUNTA_ENGINE_LOG_INFO("Resource Manager: Registered Model '{}'", name);
}

void ResourceManager::LoadTexture(const std::string& name, const std::string& path)
{
	if (texturesRegistered.count(name))
	{
		SUNTA_ENGINE_LOG_ERROR("Resource Manager: Couldn't find texture named: {}", name);
		return;
	}

	if (!rendererDevice)
	{
		SUNTA_ENGINE_LOG_ERROR("Resource Manager: Renderer device is NULL! You should call Renderer.Init() first");
		return;
	}

	auto texture = rendererDevice->CreateTexture(path);
	texturesRegistered[name] = texture;

	SUNTA_ENGINE_LOG_INFO("Resource Manager: Registered Texture '{}'", name);
}

void ResourceManager::LoadShader(const std::string& name, const std::string& path)
{
	if (shadersRegistered.count(name))
	{
		SUNTA_ENGINE_LOG_ERROR("Resource Manager: Couldn't find shader named: {}", name);
		return;
	}

	if (!rendererDevice)
	{
		SUNTA_ENGINE_LOG_ERROR("Resource Manager: Renderer device is NULL! You should call Renderer.Init() first");
		return;
	}

	auto shader = rendererDevice->CreateShader(path);
	shadersRegistered[name] = shader;

	SUNTA_ENGINE_LOG_INFO("Resource Manager: Registered Shader '{}'", name);
}

std::shared_ptr<ModelData> ResourceManager::GetModelData(const std::string& name)
{
	auto it = modelsRegistered.find(name);
	if (it == modelsRegistered.end())
	{
		SUNTA_ENGINE_LOG_ERROR("ERROR: Model '{}' not found in ResourceManager...", name);
		return nullptr;
	}
	return it->second;
}

std::shared_ptr<Texture> ResourceManager::GetTextureData(const std::string& name)
{
	auto it = texturesRegistered.find(name);
	if (it == texturesRegistered.end())
	{
		SUNTA_ENGINE_LOG_ERROR("ERROR: Texture '{}' not found in ResourceManager...", name);
		return nullptr;
	}
	return it->second;
}

std::shared_ptr<Shader> ResourceManager::GetShaderData(const std::string& name)
{
	auto it = shadersRegistered.find(name);
	if (it == shadersRegistered.end())
	{
		SUNTA_ENGINE_LOG_ERROR("ERROR: Shader '{}' not found in ResourceManager...", name);
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

		SUNTA_ENGINE_LOG_INFO("Loaded new material {0} in ResourceManager...", name);
		return newMaterial;
	}

	SUNTA_ENGINE_LOG_INFO("Loaded existing material {0} from ResourceManager...", name);
	return it->second;
}

std::shared_ptr<Texture> ResourceManager::LoadOrGetModelTexture(const std::string& path)
{
	auto it = texturesRegistered.find(path);
	if (it == texturesRegistered.end())
	{
		auto newTexture = rendererDevice->CreateTexture(path);
		texturesRegistered[path] = newTexture;

		SUNTA_ENGINE_LOG_INFO("Loaded new texture {0} in ResourceManager...", path);
		return newTexture;
	}


	SUNTA_ENGINE_LOG_INFO("Loaded existing texture{0} from ResourceManager...", path);
	return it->second;
}

}