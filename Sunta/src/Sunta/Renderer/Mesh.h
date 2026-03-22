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
		
		//(PIMPL) Pointer to IMPLementation, forward declarations
	
		//DESTRUCTOR: Must be in .cpp becuase unique_ptr needs to see the full
		//definition of VAO, VBO, EBO buffer classes to delete them (they are forward-declared)
		~Mesh();
		
		//MOVE OPERATIONS: unique_ptr cannot be copied, only moved
		//We transfer ownership of VAO, VBO, EBO from one Mesh to another
		Mesh(Mesh&& other) noexcept;
		Mesh& operator=(Mesh&& other) noexcept;
	
		//COPYING DISABLED: unique_ptr prevents copying by design
		//to ensure only Mesh object manages the VAO, VBO, EBO memory
		Mesh(const Mesh&) = delete;
		Mesh& operator=(const Mesh&) = delete;
	
	
		//void BindTextures(const Shader& shader) const;
		void Bind() const;

		inline const VertexArray&   GetVertexArrayBuffer()	const { return *vertexArray; }
		inline const ElementBuffer& GetElementBuffer()	    const { return *elementBuffer; }
		inline unsigned int         GetIndexCount()		    const { return (unsigned int)indices.size(); }
	};
}