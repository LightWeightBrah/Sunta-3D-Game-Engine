#include "Renderer.h"
#include "ElementBuffer.h"

ElementBuffer::ElementBuffer(const unsigned int* data, unsigned int size) 
{
	count = size / sizeof(unsigned int);

	GLCall(glGenBuffers(1, &id));
	GLCall(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, id));
	GLCall(glBufferData(GL_ELEMENT_ARRAY_BUFFER, size, data, GL_STATIC_DRAW));
}

ElementBuffer::~ElementBuffer()
{
	GLCall(glDeleteBuffers(1, &id));
}

void ElementBuffer::Bind() const
{
	GLCall(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, id));
}

void ElementBuffer::Unbind() const
{
	GLCall(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0));
}