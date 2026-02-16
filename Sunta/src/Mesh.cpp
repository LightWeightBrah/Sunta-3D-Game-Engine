#pragma once

#include <iostream>
#include <GL/glew.h>

#include "Mesh.h"

#include "Shader.h"
#include "Texture.h"
#include "BufferLayout.h"

#include "VertexBuffer.h"
#include "ElementBuffer.h"
#include "VertexArray.h"

Mesh::Mesh(const void* vertexData, unsigned int dataSize, std::vector<unsigned int> indices,
		std::vector<TextureItem> textures, const BufferLayout& bufferLayout)
	: indices(std::move(indices)), textures(std::move(textures))
{
	if (this->indices.empty())
	{
		std::cout << "ERROR: Mesh has no indices" << std::endl;
		return;
	}

	VAO = std::make_unique<VertexArray>();
	VBO = std::make_unique<VertexBuffer>(vertexData, dataSize);
	EBO = std::make_unique<ElementBuffer>(&this->indices[0], this->indices.size() * sizeof(unsigned int));

	VAO->AddBuffer(*VBO, bufferLayout);
}

//DESTRUCTOR: now compiler can see how to delete unique_ptr members
Mesh::~Mesh() = default;

//MOVE: transferring unique_ptr from 'other' to 'this'
Mesh::Mesh(Mesh&& other) noexcept = default;
Mesh& Mesh::operator=(Mesh&& other) noexcept = default;

void Mesh::BindTextures(const Shader& shader) const
{
	unsigned int diffuseNr  = 1;
	unsigned int specularNr = 1;

	for (unsigned int i = 0; i < textures.size(); i++)
	{
		if (!textures[i].texture)
		{
			std::cout << "ERROR: Mesh Texture at index " << i << " is null, skipping" << std::endl;
			continue;
		}

		textures[i].texture->Bind(i);

		std::string name   = textures[i].type;
		std::string number = (name == "texture_diffuse") ? std::to_string(diffuseNr++) : std::to_string(specularNr++);

		shader.SetUniform1i(name + number, i);
	}
}