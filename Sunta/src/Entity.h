#pragma once
#include <memory>

#include "Transform.h"

class Mesh;
class Material;

class Entity
{
public:
	Entity(std::shared_ptr<Mesh> mesh, std::shared_ptr<Material> material);

	Entity& SetPosition(const glm::vec3& position) { transform.SetPosition(position); return *this; }
	Entity& SetRotation(const glm::vec3& rotation) { transform.SetRotation(rotation); return *this; }
	Entity& SetScale(const glm::vec3& scale)	   { transform.SetScale(scale);		  return *this; }

	Entity& SetMesh(std::shared_ptr<Mesh> mesh)				{ this->mesh = std::move(mesh);			return *this; }
	Entity& SetMaterial(std::shared_ptr<Material> material) { this->material = std::move(material); return *this; }


	glm::vec3 GetPosition()		const	{ return transform.GetPosition();	 }
	glm::vec3 GetRotation()		const	{ return transform.GetRotation();	 }
	glm::vec3 GetScale()		const	{ return transform.GetScale();		 }
	
	glm::mat4 GetModelMatrix()			{ return transform.GetModelMatrix(); }
	
	const std::shared_ptr<Mesh>&	 GetMesh()			const { return mesh; }
	const std::shared_ptr<Material>& GetMaterial()		const { return material; }

private:
	std::shared_ptr<Mesh>			 mesh;
	std::shared_ptr<Material>		 material;
									 
	Transform						 transform;
};