#include "ResourceManager.h"
#include "Texture.h"
#include "Shader.h"

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
	std::cout << "Resource Manager: Registered Model '" << name << "'" << std::endl;
}

void ResourceManager::LoadTexture(const std::string& name, const std::string& path)
{
	if (texturesRegistered.count(name))
		return;

	auto texture = std::make_shared<Texture>(path);
	texturesRegistered[name] = texture;

	std::cout << "Resource Manager: Registered Texture '" << name << "'" << std::endl;
}

void ResourceManager::LoadShader(const std::string& name, const std::string& path)
{
	if (shadersRegistered.count(name))
		return;

	auto shader = std::make_shared<Shader>(path);
	shadersRegistered[name] = shader;

	std::cout << "Resource Manager: Registered Shader '" << name << "'" << std::endl;
}

std::shared_ptr<ModelData> ResourceManager::GetModelData(const std::string& name)
{
	auto it = modelsRegistered.find(name);
	if (it == modelsRegistered.end())
	{
		std::cout << "ERROR: Model \"" << name << "\" not found in ResourceManager..." << std::endl;
		return nullptr;
	}
	return it->second;
}

std::shared_ptr<Texture> ResourceManager::GetTextureData(const std::string& name)
{
	auto it = texturesRegistered.find(name);
	if (it == texturesRegistered.end())
	{
		std::cout << "ERROR: Texture \"" << name << "\" not found in ResourceManager..." << std::endl;
		return nullptr;
	}
	return it->second;
}

std::shared_ptr<Shader> ResourceManager::GetShaderData(const std::string& name)
{
	auto it = shadersRegistered.find(name);
	if (it == shadersRegistered.end())
	{
		std::cout << "ERROR: Shader \"" << name << "\" not found in ResourceManager..." << std::endl;
		return nullptr;
	}
	return it->second;
}