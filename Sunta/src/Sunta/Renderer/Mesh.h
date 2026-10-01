#pragma once

#include <string>
#include <memory>
#include <vector>
#include <glm/glm.hpp>

#include "Core/Config.h"

namespace Sunta
{

class RendererDevice;

class Texture;
class Shader;
	
class VertexArray;
class VertexBuffer;
class ElementBuffer;
	
class BufferLayout;
	
class Mesh	
{	
public:
	Mesh(RendererDevice& rendererDevice
		, const void* vertexData
		, unsigned int dataSize
		, std::vector<unsigned int> indices
		, const BufferLayout& bufferLayout
		, const glm::vec3& localBoundsMin = glm::vec3(-0.5f)
		, const glm::vec3& localBoundsMax = glm::vec3(0.5f));
		
	// COPYING DISABLED:
	// To ensure we don't have multiple meshes pointing to the same VAO, VBO, EBO memory
	Mesh(const Mesh&) = delete;
	Mesh& operator=(const Mesh&) = delete;
	
	
	//void BindTextures(const Shader& shader) const;
	void Bind() const;

	inline const VertexArray&   GetVertexArray()		const { return *vertexArray; }
	inline const ElementBuffer& GetElementBuffer()	    const { return *elementBuffer; }
	inline unsigned int         GetIndexCount()		    const { return (unsigned int)indices.size(); }

	inline const glm::vec3&     GetLocalBoundsMin()     const { return localBoundsMin; }
	inline const glm::vec3&     GetLocalBoundsMax()     const { return localBoundsMax; }

private:
	std::vector<unsigned int>		indices;

	std::shared_ptr<VertexArray>	vertexArray;
	std::shared_ptr<VertexBuffer>	vertexBuffer;
	std::shared_ptr<ElementBuffer>	elementBuffer;

	// For Editor Enity Picking (Raycasting)
	glm::vec3                       localBoundsMin = glm::vec3(-0.5f);
	glm::vec3                       localBoundsMax = glm::vec3(0.5f);
};

}