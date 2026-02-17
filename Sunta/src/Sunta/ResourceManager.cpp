#include "ResourceManager.h"
#include "Texture.h"
#include "Shader.h"
#include "Log.h"

namespace Sunta
{
	std::map<std::string, std::shared_ptr<ModelData>> ResourceManager::modelsRegistered;
	std::map<std::string, std::shared_ptr<Texture>>   ResourceManager::texturesRegistered;
	std::map<std::string, std::shared_ptr<Shader>>    ResourceManager::shadersRegistered;
	
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
}