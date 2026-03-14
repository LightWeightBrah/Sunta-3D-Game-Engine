#include "Core/SuntaPreCompiled.h"

#include "Renderer.h"
#include "ElementBuffer.h"

namespace Sunta
{
	OpenGLElementBuffer::OpenGLElementBuffer(const unsigned int* data, unsigned int size)
	{
		count = size / sizeof(unsigned int);
	
		GLCall(glGenBuffers(1, &id));
		GLCall(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, id));
		GLCall(glBufferData(GL_ELEMENT_ARRAY_BUFFER, size, data, GL_STATIC_DRAW));
	}
	
	OpenGLElementBuffer::~OpenGLElementBuffer()
	{
		GLCall(glDeleteBuffers(1, &id));
	}
	
	void OpenGLElementBuffer::Bind() const
	{
		GLCall(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, id));
	}
	
	void OpenGLElementBuffer::Unbind() const
	{
		GLCall(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0));
	}
}