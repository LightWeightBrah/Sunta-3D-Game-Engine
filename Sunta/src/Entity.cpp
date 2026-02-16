#include "Entity.h"
#include "Mesh.h"

Entity::Entity(std::shared_ptr<Mesh> mesh, std::shared_ptr<Material> material)
	: mesh(std::move(mesh)), material(std::move(material))
{

}