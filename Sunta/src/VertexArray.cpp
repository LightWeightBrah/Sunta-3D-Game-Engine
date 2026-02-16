#include <GL/glew.h>

#include "VertexArray.h"
#include "Renderer.h"
#include "VertexBuffer.h"
#include "BufferLayout.h"

VertexArray::VertexArray()
{
	GLCall(glGenVertexArrays(1, &id));
	Bind();
}

VertexArray::~VertexArray()
{
	GLCall(glDeleteVertexArrays(1, &id));
}

void VertexArray::AddBuffer(const VertexBuffer& VBO, const BufferLayout& layout)
{
	Bind();
	VBO.Bind();

	const auto& elements	= layout.GetBufferElements();
	unsigned int offset		= 0;

	for (unsigned int i = 0; i < elements.size(); i++)
	{
		const auto& element = elements[i];

		GLCall(glEnableVertexAttribArray(i));

		if (element.type == GL_UNSIGNED_INT)
		{
			GLCall(glVertexAttribIPointer(
				i, 
				element.count, 
				element.type, 
				layout.GetStride(), 
				(const void*)offset
			));
		}
		else
		{
			GLCall(glVertexAttribPointer(
				i,
				element.count,
				element.type,
				element.normalized ? GL_TRUE : GL_FALSE,
				layout.GetStride(),
				(const void*)offset
			));
		}

		offset += element.count * BufferElement::GetSizeOfType(element.type);
	}
}

void VertexArray::Bind() const
{
	GLCall(glBindVertexArray(id));
}

void VertexArray::Unbind() const
{
	GLCall(glBindVertexArray(0));
}
