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
private:
	std::vector<unsigned int>		indices;
	
	std::shared_ptr<VertexArray>	vertexArray;
	std::shared_ptr<VertexBuffer>	vertexBuffer;
	std::shared_ptr<ElementBuffer>	elementBuffer;
	
public:
	Mesh(RendererDevice& rendererDevice
		, const void* vertexData
		, unsigned int dataSize
		, std::vector<unsigned int> indices
		, const BufferLayout& bufferLayout);
		
	// COPYING DISABLED:
	// To ensure we don't have multiple meshes pointing to the same VAO, VBO, EBO memory
	Mesh(const Mesh&) = delete;
	Mesh& operator=(const Mesh&) = delete;
	
	
	//void BindTextures(const Shader& shader) const;
	void Bind() const;

	inline const VertexArray&   GetVertexArray()		const { return *vertexArray; }
	inline const ElementBuffer& GetElementBuffer()	    const { return *elementBuffer; }
	inline unsigned int         GetIndexCount()		    const { return (unsigned int)indices.size(); }
};

}