#include "Core/SuntaPreCompiled.h"

#include <glad/glad.h>

#include "Mesh.h"

#include "Shader.h"
#include "Texture.h"
#include "BufferLayout.h"

#include "VertexBuffer.h"
#include "ElementBuffer.h"
#include "VertexArray.h"
#include "Core/Log.h"
#include "RendererDevice.h"

namespace Sunta
{

Mesh::Mesh(RendererDevice& rendererDevice, const void* vertexData, unsigned int dataSize, std::vector<unsigned int> indices, const BufferLayout& bufferLayout)
{
	if (indices.empty())
	{
		SUNTA_ENGINE_LOG_ERROR("ERROR: Mesh has no indices");
		return;
	}
	
	vertexArray = rendererDevice.CreateVertexArray();

	BufferDescriptor vertexBufferDescriptor;
	vertexBufferDescriptor.data = vertexData;
	vertexBufferDescriptor.size = dataSize;
	vertexBufferDescriptor.usage = BufferUsage::Static;
	vertexBuffer = rendererDevice.CreateVertexBuffer(vertexBufferDescriptor);
	vertexBuffer->SetLayout(bufferLayout);


	BufferDescriptor elementBufferDescriptor;
	elementBufferDescriptor.data = indices.data();
	elementBufferDescriptor.size = indices.size() * sizeof(unsigned int);
	elementBufferDescriptor.usage = BufferUsage::Static;
	elementBuffer = rendererDevice.CreateElementBuffer(elementBufferDescriptor);
	
	vertexArray->AddVertexBuffer(vertexBuffer);
	vertexArray->SetElementBuffer(elementBuffer);

	this->indices = std::move(indices); //can move this to member initializer list, but need to use this->indices
}

void Mesh::Bind() const
{
	this->vertexArray->Bind();
	this->elementBuffer->Bind();
}

//TODO: REWORK THIS IN MATERIAL SO IT WORKS WITH 3D MODELS
//FOR MODELS
/*void Mesh::BindTextures(const Shader& shader) const
{
	unsigned int diffuseNr  = 1;
	unsigned int specularNr = 1;
	
	for (unsigned int i = 0; i < textures.size(); i++)
	{
		if (!textures[i].texture)
		{
			SUNTA_ENGINE_LOG_WARNING("Mesh Texture at index {} is null, skipping", i);
			continue;
		}
	
		textures[i].texture->Bind(i);
	
		std::string name   = textures[i].type;
		std::string number = (name == "texture_diffuse") ? std::to_string(diffuseNr++) : std::to_string(specularNr++);
	
		shader.SetUniform1i(name + number, i);
	}
}*/

}