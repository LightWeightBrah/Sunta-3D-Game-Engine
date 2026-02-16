#include <utility>

#include "Material.h"
#include "Shader.h"

Material::Material(
	std::shared_ptr<Shader> shader)
	: 
	shader(std::move(shader))
{

}

//DESTUCTOR: now compiler can see how to delete Shader shared_ptr member
Material::~Material() = default;

Material& Material::ApplyLight()
{
	if (!shader)
		return *this;

	shader->Bind();
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

std::shared_ptr<Shader> Material::GetShader() const { return shader; }