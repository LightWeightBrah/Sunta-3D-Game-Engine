#pragma once

#include <string>
#include <memory>
#include <vector>
#include <glm/glm.hpp>

#include "Config.h"

class Texture;
class Shader;

class VertexArray;
class VertexBuffer;
class ElementBuffer;

class BufferLayout;

struct TextureItem 
{
	std::shared_ptr<Texture> texture;
	std::string				 type;
};

class Mesh	
{
private:
	std::vector<unsigned int>		indices;
	std::vector<TextureItem>		textures;

	std::unique_ptr<VertexArray>	VAO;
	std::unique_ptr<VertexBuffer>	VBO;
	std::unique_ptr<ElementBuffer>	EBO;

public:
	Mesh(const void* vertexData, unsigned int dataSize, std::vector<unsigned int> indices, 
		std::vector<TextureItem> textures, const BufferLayout& bufferLayout);
	
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


	void BindTextures(const Shader& shader) const;

	inline const VertexArray& GetVAO()		const { return *VAO; }
	inline const ElementBuffer& GetEBO()	const { return *EBO; }
	inline unsigned int GetIndexCount()		const { return (unsigned int)indices.size(); }
};