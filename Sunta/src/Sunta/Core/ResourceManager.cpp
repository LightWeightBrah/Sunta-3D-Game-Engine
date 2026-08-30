#include "SuntaPreCompiled.h"
#include "ResourceManager.h"

#include "Log.h"
#include "Renderer/Texture.h"
#include "Renderer/Shader.h"
#include "Renderer/Material.h"
#include "Renderer/Model.h"
#include "Animation/Animation.h"
#include "Renderer/RendererDevice.h"
#include "Serialization/MaterialSerializer.h"
#include "Utilities/FileSystemUtilities.h"
#include "VirtualFileSystem.h"

namespace Sunta
{

void ResourceManager::LoadEditorIcon(const std::string& name, const std::string& filepath)
{
	std::string resolvedPath = VirtualFileSystem::Resolve(filepath);

	if (editorIconsRegistered.count(name))
	{
		SUNTA_ENGINE_LOG_ERROR("ResourceManager::LoadEditorIcon: Editor Icon with name '{0}' (filepath: '{1}') already exists! Cannot map multiple filepaths to the same name!", name, resolvedPath);
		return;
	}

	if (!rendererDevice)
	{
		SUNTA_ENGINE_LOG_ERROR("Resource Manager: Renderer device is NULL! You should call Renderer.Init() first");
		return;
	}

	auto iconTexture = rendererDevice->CreateTexture(resolvedPath, false);
	iconTexture->SetName(name);
	editorIconsRegistered[name] = iconTexture;

	SUNTA_ENGINE_LOG_INFO("Resource Manager: Registered Editor Icon: '{0}' filepath: '{1}'", name, resolvedPath);
}

void ResourceManager::LoadModel(const std::string& name, const std::string& filepath, float importScale)
{
	std::string resolvedPath = VirtualFileSystem::Resolve(filepath);

	if (modelsRegistered.count(name))
	{
		SUNTA_ENGINE_LOG_ERROR("ResourceManager::LoadModel: Model with name '{0}' (filepath: '{1}') already exists! Cannot map multiple filepaths to the same name!", name, resolvedPath);
		return;
	}

	if (!rendererDevice)
	{
		SUNTA_ENGINE_LOG_ERROR("Resource Manager: Renderer device is NULL! You should call Renderer.Init() first");
		return;
	}

	auto data = std::make_shared<ModelData>();
	data->model = std::make_shared<Model>(*rendererDevice, resolvedPath, false, importScale);

	const aiScene* scene = data->model->GetScene();
	if (!scene)
	{
		SUNTA_ENGINE_LOG_ERROR("ResourceManager::LoadModel: Failed to Load model '{0}' from: '{1}'! See Assimp error Above!", name, resolvedPath);
		return;

	}

	auto animationNames = Animation::GetAnimationsNames(scene);

	for (unsigned int i = 0; i < animationNames.size(); i++)
	{
		Animation animation(scene, i);
			if (animation.IsValid())
				data->animations[animationNames[i]] = animation;
	}

	modelsRegistered[name] = data;
	SUNTA_ENGINE_LOG_INFO("Resource Manager: Registered Model: '{0}' filepath: '{1}'", name, resolvedPath);
}

void ResourceManager::LoadMesh(const std::string& name, std::function<std::shared_ptr<Mesh>()> primitiveFactory)
{
	if (meshesRegistered.count(name))
	{
		SUNTA_ENGINE_LOG_ERROR("ResourceManager::LoadMesh: Mesh with name '{0}' already exists! Cannot map multiple meshes to the same name!", name);
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
	if (!material)
	{
		SUNTA_ENGINE_LOG_ERROR("Resource Manager: Material passed to LoadMaterial is NULL!");
		return;
	}

	if (materialsRegistered.count(name))
	{
		SUNTA_ENGINE_LOG_ERROR("ResourceManager::LoadMaterial: Material with name '{0}' already exists! Cannot map multiple materials to the same name!", name);
		return;
	}

	material->SetName(name);
	materialsRegistered[name] = material;

	SUNTA_ENGINE_LOG_INFO("Resource Manager: Registered Material: '{0}'", name);
}

void ResourceManager::LoadTexture(const std::string& name, const std::string& filepath)
{
	std::string resolvedPath = VirtualFileSystem::Resolve(filepath);

	if (texturesRegistered.count(name))
	{
		SUNTA_ENGINE_LOG_ERROR("ResourceManager::LoadTexture: Texture with name '{0}' (filepath: '{1}') already exists! Cannot map multiple filepaths to the same name!", name, resolvedPath);
		return;
	}

	if (!rendererDevice)
	{
		SUNTA_ENGINE_LOG_ERROR("Resource Manager: Renderer device is NULL! You should call Renderer.Init() first");
		return;
	}

	auto texture = rendererDevice->CreateTexture(resolvedPath);
	texture->SetName(name);
	texturesRegistered[name] = texture;

	SUNTA_ENGINE_LOG_INFO("Resource Manager: Registered Texture: '{0}' filepath: '{1}'", name, resolvedPath);
}

void ResourceManager::LoadShader(const std::string& name, const std::string& filepath)
{
	std::string resolvedPath = VirtualFileSystem::Resolve(filepath);

	if (shadersRegistered.count(name))
	{
		SUNTA_ENGINE_LOG_ERROR("ResourceManager::LoadShader: Shader with name '{0}' (filepath: '{1}') already exists! Cannot map multiple filepaths to the same name!", name, resolvedPath);
		return;
	}

	if (!rendererDevice)
	{
		SUNTA_ENGINE_LOG_ERROR("Resource Manager: Renderer device is NULL! You should call Renderer.Init() first");
		return;
	}

	auto shader = rendererDevice->CreateShader(resolvedPath);
	shader->SetName(name);
	shadersRegistered[name] = shader;

	SUNTA_ENGINE_LOG_INFO("Resource Manager: Registered Shader: '{0}' filepath: '{1}'", name, resolvedPath);
}

void ResourceManager::UnloadEditorIcon(const std::string& name)
{
	if (editorIconsRegistered.count(name))
	{
		editorIconsRegistered.erase(name);
		SUNTA_ENGINE_LOG_INFO("ResourceManager::UnloadEditorIcon: Unloaded Editor Icon: '{0}'", name);
	}
}

void ResourceManager::UnloadModel(const std::string& name)
{
	if (modelsRegistered.count(name))
	{
		modelsRegistered.erase(name);
		SUNTA_ENGINE_LOG_INFO("ResourceManager::UnloadModel: Unloaded 3D Model: '{0}'", name);
	}
}

void ResourceManager::UnloadMesh(const std::string& name)
{
	if (meshesRegistered.count(name))
	{
		meshesRegistered.erase(name);
		SUNTA_ENGINE_LOG_INFO("ResourceManager::UnloadMesh: Unloaded Mesh: '{0}'", name);
	}
}

void ResourceManager::UnloadMaterial(const std::string& name)
{
	if (materialsRegistered.count(name))
	{
		materialsRegistered.erase(name);
		SUNTA_ENGINE_LOG_INFO("ResourceManager::UnloadMaterial: Unloaded Material: '{0}'", name);
	}
}

void ResourceManager::UnloadTexture(const std::string& name)
{
	if (texturesRegistered.count(name))
	{
		texturesRegistered.erase(name);
		SUNTA_ENGINE_LOG_INFO("ResourceManager::UnloadTexture: Unloaded Texture: '{0}'", name);
	}
}

void ResourceManager::UnloadShader(const std::string& name)
{
	if (shadersRegistered.count(name))
	{
		shadersRegistered.erase(name);
		SUNTA_ENGINE_LOG_INFO("ResourceManager::UnloadShader: Unloaded Shader: '{0}'", name);
	}
}

void ResourceManager::UnloadResourceByPath(const std::filesystem::path& filePath)
{
	std::string extension = filePath.extension().string();
	std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);

	std::string stemName = filePath.stem().string();
	std::string fullPathString = filePath.generic_string();

	if (IsMaterialExtension(extension))
	{
		UnloadMaterial(stemName);
	}
	else if (IsTextureExtension(extension))
	{
		UnloadTexture(stemName);
		UnloadTexture(fullPathString);
	}
	else if (IsShaderExtension(extension))
	{
		UnloadShader(stemName);
	}
	else if (IsModelExtension(extension))
	{
		UnloadModel(stemName);
	}
}

void ResourceManager::RenameMaterial(const std::string& oldName, const std::string& newName)
{
	if (oldName == newName)
		return;

	if (materialsRegistered.count(newName) > 0)
	{
		SUNTA_ENGINE_LOG_WARNING("ResourceManager::RenameMaterial: Material named '{0}' already exists!", newName);
		return;
	}

	auto it = materialsRegistered.find(oldName);
	if (it != materialsRegistered.end())
	{
		auto material = it->second;

		materialsRegistered.erase(it);

		material->SetName(newName);

		materialsRegistered[newName] = material;

		SUNTA_ENGINE_LOG_INFO("ResourceManager::RenameMaterial: Renamed Material from '{0}' to '{1}'", oldName, newName);
	}
}

std::shared_ptr<Texture> ResourceManager::GetEditorIcon(const std::string& name)
{
	auto it = editorIconsRegistered.find(name);
	if (it == editorIconsRegistered.end())
	{
		SUNTA_ENGINE_LOG_ERROR("ResourceManager::GetEditorIcon: Couldn't find Editor Icon named: '{0}'", name);
		return nullptr;
	}
	return it->second;
}

std::shared_ptr<ModelData> ResourceManager::GetModelData(const std::string& name)
{
	auto it = modelsRegistered.find(name);
	if (it == modelsRegistered.end())
	{
		SUNTA_ENGINE_LOG_ERROR("ResourceManager::GetModelData: Couldn't find Model named: '{0}'", name);
		return nullptr;
	}
	return it->second;
}

std::shared_ptr<Mesh> ResourceManager::GetMeshData(const std::string& name)
{
	auto it = meshesRegistered.find(name);
	if (it == meshesRegistered.end())
	{
		SUNTA_ENGINE_LOG_ERROR("ResourceManager::GetMeshData: Couldn't find Mesh named: '{0}'", name);
		return nullptr;
	}
	return it->second;
}

std::shared_ptr<Texture> ResourceManager::GetTextureData(const std::string& name)
{
	auto it = texturesRegistered.find(name);
	if (it == texturesRegistered.end())
	{
		SUNTA_ENGINE_LOG_ERROR("ResourceManager::GetTextureData: Couldn't find Texture named: '{0}'", name);
		return nullptr;
	}
	return it->second;
}

std::shared_ptr<Shader> ResourceManager::GetShaderData(const std::string& name)
{
	auto it = shadersRegistered.find(name);
	if (it == shadersRegistered.end())
	{
		SUNTA_ENGINE_LOG_ERROR("ResourceManager::GetShaderData: Couldn't find Shader named: '{0}'", name);
		return nullptr;
	}
	return it->second;
}

std::shared_ptr<Material> ResourceManager::GetMaterialData(const std::string& name)
{
	auto it = materialsRegistered.find(name);
	if (it == materialsRegistered.end())
	{
		SUNTA_ENGINE_LOG_ERROR("ResourceManager::GetMaterialData: Couldn't find Material named: '{0}'", name);
		return nullptr;
	}
	return it->second;
}

std::shared_ptr<Material> ResourceManager::LoadOrGetMaterial(const std::string& name, std::shared_ptr<Shader> shader)
{
	auto it = materialsRegistered.find(name);
	if (it != materialsRegistered.end())
	{
		SUNTA_ENGINE_LOG_INFO("ResourceManager::LoadOrGetMaterial: Loaded existing Material '{0}'", name);
		return it->second;
	}

	auto newMaterial = std::make_shared<Material>(shader);
	LoadMaterial(name, newMaterial);
	return newMaterial;
}

// For automatic textures e.g 3d Models
std::shared_ptr<Texture> ResourceManager::LoadOrGetTexture(const std::string& filepath)
{
	std::string resolvedPath = VirtualFileSystem::Resolve(filepath);

	auto it = texturesRegistered.find(resolvedPath);
	if (it != texturesRegistered.end())
	{
		SUNTA_ENGINE_LOG_INFO("ResourceManager::LoadOrGetTexture: Loaded existing Texture '{0}'", resolvedPath);
		return it->second;
	}

	LoadTexture(resolvedPath, resolvedPath);
	return GetTextureData(resolvedPath);
}

std::shared_ptr<Material> ResourceManager::LoadMaterialFromFile(const std::string& filepath)
{
	std::string resolvedPath = VirtualFileSystem::Resolve(filepath);

	std::filesystem::path path(resolvedPath);
	std::string materialName = path.stem().string();

	auto it = materialsRegistered.find(materialName);
	if (it != materialsRegistered.end())
		return it->second;

	auto deserializedMaterial = MaterialSerializer::Deserialize(resolvedPath);
	if (deserializedMaterial)
	{
		LoadMaterial(materialName, deserializedMaterial);
	}

	return deserializedMaterial;
}

std::vector<std::string> ResourceManager::GetMeshesNames()
{
	std::vector<std::string> names;
	for (const auto& [name, mesh] : meshesRegistered)
		names.push_back(name);

	return names;

}

std::vector<std::string> ResourceManager::GetModelsNames()
{
	std::vector<std::string> names;
	for (const auto& [name, model] : modelsRegistered)
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