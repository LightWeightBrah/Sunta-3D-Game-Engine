#include "SuntaPreCompiled.h"

#include "Material.h"
#include "Texture.h"
#include "Shader.h"
#include "ResourceManager.h"

namespace Sunta
{
	Material::Material(
		std::shared_ptr<Shader> shader, std::shared_ptr<Texture> diffuseMap, std::shared_ptr<Texture> specularMap)
		: shader(std::move(shader))
		, diffuseMap(std::move(diffuseMap))
		, specularMap(std::move(specularMap))

	{
		if(!diffuseMap)
			this->SetDiffuseMap(ResourceManager::GetTextureData("whiteTexture"));
		if(!specularMap)
			this->SetSpecularMap(ResourceManager::GetTextureData("whiteTexture"));
	}
	
	//DESTUCTOR: now compiler can see how to delete Shader shared_ptr member
	Material::~Material() = default;
	
	Material& Material::ApplyLight()
	{
		if (!shader)
			return *this;
	
		shader->Bind();
		diffuseMap->Bind(diffuseTextureSlot);
		specularMap->Bind(specularTextureSlot);

		shader->SetUniform3f("material.ambientColor",	data.ambientColor);
		shader->SetUniform3f("material.diffuseColor",	data.diffuseColor);
		shader->SetUniform3f("material.specularColor",	data.specularColor);
		shader->SetUniform1f("material.shininess",		data.shininess);

		return *this;
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

	Material& Material::SetDiffuseMap(const std::shared_ptr<Texture> diffuseMap)
	{
		this->diffuseMap = diffuseMap;

		if (!shader)
			return *this;

		shader->Bind();
		shader->SetUniform1i("materialDiffuseMap", this->diffuseTextureSlot);

		return *this;
	}

	Material& Material::SetSpecularMap(const std::shared_ptr<Texture> specularMap)
	{
		this->specularMap = specularMap;

		if (!shader)
			return *this;

		shader->Bind();
		shader->SetUniform1i("materialSpecularMap", this->specularTextureSlot);

		return *this;
	}
	
	std::shared_ptr<Shader> Material::GetShader() const { return shader; }
}