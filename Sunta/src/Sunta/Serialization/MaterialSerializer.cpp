#include "Core/SuntaPreCompiled.h"
#include "MaterialSerializer.h"

#include <fstream>
#include <nlohmann/json.hpp>

#include "Renderer/Material.h"
#include "Renderer/Shader.h"
#include "Renderer/Texture.h"
#include "Core/ResourceManager.h"
#include "Core/Log.h"
#include "Core/EngineAssets.h"

namespace Sunta
{

using json = nlohmann::json;

namespace MaterialKeys
{
	constexpr const char* Name           = "name";
	constexpr const char* Shader         = "shader";
	constexpr const char* Properties     = "properties";
	constexpr const char* Ambient        = "ambient";
	constexpr const char* Diffuse        = "diffuse";
	constexpr const char* Specular       = "specular";
	constexpr const char* Shininess      = "shininess";
	constexpr const char* Textures       = "textures";
	constexpr const char* DiffuseMaps    = "diffuse_maps";
	constexpr const char* SpecularMaps   = "specular_maps";
	constexpr const char* UseAlphaCutout = "use_alpha_cutout";

}


bool MaterialSerializer::Serialize(const std::string& filepath, const std::shared_ptr<Material>& material)
{
	if (!material)
	{
		SUNTA_ENGINE_LOG_ERROR("MaterialSerializer: Attempted to Serialize a NULL material!");
		return false;
	}

	json data;

	data[MaterialKeys::Name] = material->GetName();

	data[MaterialKeys::Shader] = material->GetShader() ? material->GetShader()->GetName() : Sunta::EngineAssets::Shaders::Lit;

	const auto& materialData = material->GetData();
	data[MaterialKeys::Properties][MaterialKeys::Ambient]   = { materialData.ambientColor.r,  materialData.ambientColor.g,  materialData.ambientColor.b };
	data[MaterialKeys::Properties][MaterialKeys::Diffuse]   = { materialData.diffuseColor.r,  materialData.diffuseColor.g,  materialData.diffuseColor.b };
	data[MaterialKeys::Properties][MaterialKeys::Specular]  = { materialData.specularColor.r, materialData.specularColor.g, materialData.specularColor.b };
	data[MaterialKeys::Properties][MaterialKeys::Shininess] =   materialData.shininess;

	data[MaterialKeys::Properties][MaterialKeys::UseAlphaCutout] = materialData.useAlphaCutout;

	json diffusePaths = json::array();
	for (const auto& texture : material->GetDiffuseMaps())
	{
		if (texture)
			diffusePaths.push_back(texture->GetFilePath());
	}
	data[MaterialKeys::Textures][MaterialKeys::DiffuseMaps] = diffusePaths;

	json specularPaths = json::array();
	for (const auto& texture : material->GetSpecularMaps())
	{
		if (texture)
			specularPaths.push_back(texture->GetFilePath());
	}
	data[MaterialKeys::Textures][MaterialKeys::SpecularMaps] = specularPaths;

	std::ofstream outFile(filepath);
	if (!outFile.is_open())
	{
		SUNTA_ENGINE_LOG_ERROR("MaterialSerializer::Serialize: Couldn't open file for writing: {0}", filepath);
		return false;
	}

	// Save formatted JSON to file (with 4 space indent)
	outFile << data.dump(4);
	outFile.close();

	SUNTA_ENGINE_LOG_INFO("MaterialSerializer::Serialize: Successfully saved material '{0}' to '{1}'", material->GetName(), filepath);
	return true;

}

std::shared_ptr<Material> MaterialSerializer::Deserialize(const std::string& filepath)
{
	std::ifstream inFile(filepath);
	if (!inFile.is_open())
	{
		SUNTA_ENGINE_LOG_ERROR("MaterialSerializer::Deserialize: Couldn't open file: '{0}'", filepath);
		return nullptr;
	}

	json data;

	try
	{
		inFile >> data;
	}
	catch (const json::parse_error& error)
	{
		SUNTA_ENGINE_LOG_ERROR("MaterialSerializer::Deserialize: JSON Parse Error in '{0}' : '{1}'", filepath, error.what());
		return nullptr;
	}

	inFile.close();

	std::string shaderName = data.value(MaterialKeys::Shader, "");
	auto shader = ResourceManager::GetShaderData(shaderName);

	if (!shader)
	{
		SUNTA_ENGINE_LOG_WARNING("MaterialSerializer::Deserialize: Shader '{0}' not found, using Error shader instead", shaderName);
		shader = ResourceManager::GetShaderData(Sunta::EngineAssets::Shaders::Error);
	}

	auto material = std::make_shared<Material>(shader);
	material->SetName(data.value(MaterialKeys::Name, "unnamed_material"));

	if (data.contains(MaterialKeys::Properties))
	{
		const auto& properties = data[MaterialKeys::Properties];

		if (properties.contains(MaterialKeys::Ambient))
			material->SetAmbient({ properties[MaterialKeys::Ambient][0], properties[MaterialKeys::Ambient][1], properties[MaterialKeys::Ambient][2] });

		if (properties.contains(MaterialKeys::Diffuse))
			material->SetDiffuse({ properties[MaterialKeys::Diffuse][0], properties[MaterialKeys::Diffuse][1], properties[MaterialKeys::Diffuse][2] });

		if (properties.contains(MaterialKeys::Specular))
			material->SetSpecular({ properties[MaterialKeys::Specular][0], properties[MaterialKeys::Specular][1], properties[MaterialKeys::Specular][2] });

		if (properties.contains(MaterialKeys::Shininess))
			material->SetShininess(properties[MaterialKeys::Shininess]);

		if (properties.contains(MaterialKeys::UseAlphaCutout))
			material->SetAlphaCutout(properties[MaterialKeys::UseAlphaCutout]);
	}

	if (data.contains(MaterialKeys::Textures))
	{
		const auto& textures = data[MaterialKeys::Textures];

		if (textures.contains(MaterialKeys::DiffuseMaps))
		{
			for (const auto& path : textures[MaterialKeys::DiffuseMaps])
			{
				if(auto diffuseTexture = ResourceManager::LoadOrGetTexture(path.get<std::string>()))
					material->AddDiffuseMap(diffuseTexture);
			}
		}

		if (textures.contains(MaterialKeys::SpecularMaps))
		{
			for (const auto& path : textures[MaterialKeys::SpecularMaps])
			{
				if(auto specularTexture = ResourceManager::LoadOrGetTexture(path.get<std::string>()))
					material->AddSpecularMap(specularTexture);
			}
		}
	}

	SUNTA_ENGINE_LOG_INFO("MaterialSerializer::Deserialize: Successfully loaded material '{0}' from '{1}'", material->GetName(), filepath);
	return material;
}


}