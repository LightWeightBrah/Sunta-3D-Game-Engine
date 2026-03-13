#include "SuntaPreCompiled.h"

#include "ResourceManager.h"
#include "Renderer/Texture.h"
#include "Renderer/Shader.h"
#include "Log.h"
#include "Renderer/Material.h"

namespace Sunta
{
	std::map<std::string, std::shared_ptr<ModelData>> ResourceManager::modelsRegistered;
	std::map<std::string, std::shared_ptr<Texture>>   ResourceManager::texturesRegistered;
	std::map<std::string, std::shared_ptr<Shader>>    ResourceManager::shadersRegistered;
	std::map<std::string, std::shared_ptr<Material>>  ResourceManager::materialsRegistered;

	void ResourceManager::LoadModel(const std::string& name, const std::string& path)
	{
		if (modelsRegistered.count(name))
			return;
	
		auto data = std::make_shared<ModelData>();
	
		data->model = Model(path, false);
	
		data->animations[AnimationType::IDLE]	 = Animation(path, &data->model, 0);
		data->animations[AnimationType::GESTURE] = Animation(path, &data->model, 1);
		data->animations[AnimationType::RUNNING] = Animation(path, &data->model, 2);
	
		modelsRegistered[name] = data;
		SUNTA_ENGINE_LOG_INFO("Resource Manager: Registered Model '{}'", name);
	}
	
	void ResourceManager::LoadTexture(const std::string& name, const std::string& path)
	{
		if (texturesRegistered.count(name))
			return;
	
		auto texture = std::make_shared<Texture>(path);
		texturesRegistered[name] = texture;
	
		SUNTA_ENGINE_LOG_INFO("Resource Manager: Registered Texture '{}'", name);
	}
	
	void ResourceManager::LoadShader(const std::string& name, const std::string& path)
	{
		if (shadersRegistered.count(name))
			return;
	
		auto shader = std::make_shared<Shader>(path);
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
			auto newTexture = std::make_shared<Texture>(path);
			texturesRegistered[path] = newTexture;

			SUNTA_ENGINE_LOG_INFO("Loaded new texture {0} in ResourceManager...", path);
			return newTexture;
		}


		SUNTA_ENGINE_LOG_INFO("Loaded existing texture{0} from ResourceManager...", path);
		return it->second;
	}

}