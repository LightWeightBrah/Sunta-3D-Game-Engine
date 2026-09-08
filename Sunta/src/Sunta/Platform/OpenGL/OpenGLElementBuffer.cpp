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
	// Make sure you create OpenGLElementBuffer only after binding VAO
	// VAO stores Element Buffer

#ifdef SUNTA_DEBUG
	int boundVertexArray = 0;
	glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &boundVertexArray);
	SUNTA_ASSERT(boundVertexArray != 0, "You are creating Element Buffer without bound Vertex Array! You must bind VAO before creating Element Buffer, use VertexArray Bind or ScopedVertexArrayBind!");
#endif

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