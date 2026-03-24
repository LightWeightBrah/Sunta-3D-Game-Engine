#include "Core/SuntaPreCompiled.h"
#include "OpenGLElementBuffer.h"

#include <glad/glad.h>

#include "OpenGLUtilities.h"
#include "Renderer/RendererDevice.h"

namespace Sunta
{

OpenGLElementBuffer::OpenGLElementBuffer(const BufferDescriptor& descriptor)
	: count(descriptor.size / sizeof(unsigned int))
{
	GLCall(glGenBuffers(1, &id));
	GLCall(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, id));
	GLCall(glBufferData(GL_ELEMENT_ARRAY_BUFFER, descriptor.size, descriptor.data, BufferUsageToOpenGL(descriptor.usage)));
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