#include "LightSource.h"
#include "Shader.h"

LightSource::LightSource(std::shared_ptr<Mesh> mesh, std::shared_ptr<Shader> shader)
	: mesh(std::move(mesh))
	, shader(std::move(shader))
{

}

LightSource& LightSource::ApplyLight(Shader& shaderToLightUp)
{
	shaderToLightUp.Bind();
	shaderToLightUp.SetUniform3f("lightSource.position",			transform.GetPosition());
	
	shaderToLightUp.SetUniform3f("lightSource.ambientIntensity",	data.ambientIntensity);
	shaderToLightUp.SetUniform3f("lightSource.diffuseIntensity",	data.diffuseIntensity);
	shaderToLightUp.SetUniform3f("lightSource.specularIntensity",	data.specularIntensity);

	return *this;
}