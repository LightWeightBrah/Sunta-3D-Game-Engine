#pragma once

#include <memory>
#include <glm/glm.hpp>

namespace Sunta
{

class Shader;
class Texture;
	
class Material
{
public:
	struct Data
	{
		glm::vec3		ambientColor		= glm::vec3(1.0f);
		glm::vec3		diffuseColor		= glm::vec3(1.0f);
		glm::vec3		specularColor		= glm::vec3(1.0f);

		float			shininess			= 32.0f;
	};
	
	Material(std::shared_ptr<Shader> shader
		, std::shared_ptr<Texture> diffuseMap = nullptr
		, std::shared_ptr<Texture> specularMap = nullptr);
		
	//DESTRUCTOR: same thing like in Mesh.h, compiler need to know
	//how to delete the Shader in shared_ptr so we gotta know the Shader destructor
	~Material();
	
	Material& SetAmbient(const glm::vec3& color);
	Material& SetDiffuse(const glm::vec3& color);
	Material& SetSpecular(const glm::vec3& color);
	Material& SetShininess(float shininess);
	
	Material& Material::SetDiffuseMap(const std::shared_ptr<Texture> diffuseMap);
	Material& Material::SetSpecularMap(const std::shared_ptr<Texture> specularMap);

	Material& ApplyLight();
	
	inline const Data&			GetData()		const { return data; }
	std::shared_ptr<Shader>		GetShader()		const;
	
private:
	std::shared_ptr<Shader>		shader;

	std::shared_ptr<Texture>	diffuseMap;
	std::shared_ptr<Texture>	specularMap;
	unsigned int				diffuseTextureSlot	= 0;
	unsigned int				specularTextureSlot = 1;

	Data						data;
};

}