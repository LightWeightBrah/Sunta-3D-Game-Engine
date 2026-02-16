#pragma once

#include <memory>

#include "Transform.h"

class Shader;
class Mesh;

class LightSource
{
public:
	struct Data
	{
		glm::vec3		ambientIntensity		= glm::vec3(0.2f);
		glm::vec3		diffuseIntensity		= glm::vec3(0.5f);
		glm::vec3		specularIntensity		= glm::vec3(1.0f);
	};

	LightSource(std::shared_ptr<Mesh> mesh, std::shared_ptr<Shader> shader);

	LightSource& ApplyLight(Shader& shaderToLightUp);

	LightSource& SetAmbient (const glm::vec3& ambient)	{ data.ambientIntensity  = ambient;	 return *this; }
	LightSource& SetDiffuse (const glm::vec3& diffuse)	{ data.diffuseIntensity	 = diffuse;	 return *this; }
	LightSource& SetSpecular(const glm::vec3& specular) { data.specularIntensity = specular; return *this; }
	
	LightSource& SetPosition(const glm::vec3& position) { transform.SetPosition(position);	 return *this; }
	LightSource& SetRotation(const glm::vec3& rotation) { transform.SetRotation(rotation);	 return *this; }
	LightSource& SetScale(const glm::vec3& scale)		{ transform.SetScale(scale);		 return *this; }

	LightSource& SetMesh(std::shared_ptr<Mesh> mesh)	{ this->mesh = std::move(mesh);		 return *this; }
	
	inline const Data&			    GetData()		const { return data;	}
	inline std::shared_ptr<Shader>	GetShader()		const { return shader;	}
	inline std::shared_ptr<Mesh>    GetMesh()		const { return mesh;	}

	glm::vec3 GetPosition()		const { return transform.GetPosition();		}
	glm::vec3 GetRotation()		const { return transform.GetRotation();		}
	glm::vec3 GetScale()		const { return transform.GetScale();		}
	
	glm::mat4 GetModelMatrix()		  { return transform.GetModelMatrix();	}

private:
	std::shared_ptr<Shader>			shader;
	std::shared_ptr<Mesh>			mesh;

	Transform						transform;
	Data							data;
};