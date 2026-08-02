#include "Core/SuntaPreCompiled.h"

#include "Material.h"
#include "Texture.h"
#include "Shader.h"
#include "Core/ResourceManager.h"
#include "Core/Log.h"

namespace Sunta
{

Material::Material(std::shared_ptr<Shader> shader)
	: shader(std::move(shader))
{

}

Material::Material(
	std::shared_ptr<Shader> shader, std::shared_ptr<Texture> diffuseMap, std::shared_ptr<Texture> specularMap)
	: shader(std::move(shader))
{
	this->AddDiffuseMap(diffuseMap);
	this->AddSpecularMap(specularMap);
}
	
//DESTUCTOR: now compiler can see how to delete Shader shared_ptr member
Material::~Material() = default;
	
Material& Material::Apply()
{
	if (!shader )
	{
		SUNTA_ENGINE_LOG_ERROR("Material: Material has no shader!");
		return *this;
	}

	shader->Bind();
	unsigned int textureSlot = 0;

	ApplyTextures(diffuseMaps, "materialDiffuseMap", textureSlot);
	ApplyTextures(specularMaps, "materialSpecularMap", textureSlot);
		
	shader->SetUniform3f("material.ambientColor",	data.ambientColor);
	shader->SetUniform3f("material.diffuseColor",	data.diffuseColor);
	shader->SetUniform3f("material.specularColor",	data.specularColor);
	shader->SetUniform1f("material.shininess",		data.shininess);

	for (const auto& [name, value] : customVec3Uniforms)
	{
		shader->SetUniform3f(name, value);
	}

	return *this;
}

void Material::ApplyTextures(const std::vector<std::shared_ptr<Texture>>& maps, const std::string& baseName, unsigned int& textureSlot)
{
	if (maps.empty())
	{
		if (auto whiteTexture = ResourceManager::GetTextureData("whiteTexture"))
		{
			whiteTexture->Bind(textureSlot);
			shader->SetUniform1i(baseName + "1", textureSlot++);
		}
			
		return;
	}

	for (unsigned int i = 0; i < maps.size(); i++)
	{
		if (!maps[i])
		{
			SUNTA_ENGINE_LOG_ERROR("Material: Texture map {0} at index {1} is null", baseName, i);
				
			if (auto errorTexture = ResourceManager::GetTextureData("errorTexture"))
			{
				errorTexture->Bind(textureSlot);
				shader->SetUniform1i(baseName + std::to_string(i + 1), textureSlot++);
			}

			continue;
		}

		maps[i]->Bind(textureSlot);
		shader->SetUniform1i(baseName + std::to_string(i + 1), textureSlot++);
	}
}
	
Material& Material::SetAmbient(const glm::vec3& color)
{
	data.ambientColor = color;
	return *this;
}
	
Material& Material::SetDiffuse(const glm::vec3& color)
{
	data.diffuseColor = color;
	return *this;
}
	
Material& Material::SetSpecular(const glm::vec3& color)
{
	data.specularColor = color;
	return *this;
}
	
Material& Material::SetShininess(float shininess)
{
	data.shininess = shininess;
	return *this;
}

Material& Material::AddDiffuseMap(const std::shared_ptr<Texture> diffuseMap)
{
	this->diffuseMaps.push_back(diffuseMap);
	return *this;
}

Material& Material::AddSpecularMap(const std::shared_ptr<Texture> specularMap)
{
	this->specularMaps.push_back(specularMap);
	return *this;
}
	
Material& Material::SetUniform3f(const std::string& name, const glm::vec3& value)
{
	customVec3Uniforms[name] = value;
	return *this;
}

std::shared_ptr<Shader> Material::GetShader() const { return shader; }

}