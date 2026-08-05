#pragma once

#include <memory>
#include <glm/glm.hpp>
#include <unordered_map>

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
	
	Material(std::shared_ptr<Shader> shader);

	Material(std::shared_ptr<Shader> shader
		, std::shared_ptr<Texture> diffuseMap
		, std::shared_ptr<Texture> specularMap);
		
	//DESTRUCTOR: same thing like in Mesh.h, compiler need to know
	//how to delete the Shader in shared_ptr so we gotta know the Shader destructor
	~Material();
	
	inline const std::string& GetName() const { return name; }
	inline void SetName(const std::string& name) { this->name = name; }

	Material& SetAmbient(const glm::vec3& color);
	Material& SetDiffuse(const glm::vec3& color);
	Material& SetSpecular(const glm::vec3& color);
	Material& SetShininess(float shininess);
	
	Material& AddDiffuseMap(const std::shared_ptr<Texture> diffuseMap);
	Material& AddSpecularMap(const std::shared_ptr<Texture> specularMap);
	
	Material& SetUniform3f(const std::string& name, const glm::vec3& value);

	Material& Apply();
	
	inline const Data&			GetData()		const { return data; }
	std::shared_ptr<Shader>		GetShader()		const;

	bool						NeedsLoading()	const { return diffuseMaps.empty() && specularMaps.empty(); }
	
private:
	std::string name = "";

	std::shared_ptr<Shader>		shader;

	std::vector<std::shared_ptr<Texture>> diffuseMaps;
	std::vector<std::shared_ptr<Texture>> specularMaps;

	std::unordered_map<std::string, glm::vec3> customVec3Uniforms;

	Data						data;

	void ApplyTextures(const std::vector<std::shared_ptr<Texture>>& maps, const std::string& baseName, unsigned int& textureSlot);
};

}