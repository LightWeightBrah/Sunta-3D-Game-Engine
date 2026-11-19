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
	// The element buffer binding is part of the bound vertex array's state: filling the new buffer while some VAO is
	// bound would silently replace that VAO's triangles (a draw leaves its VAO bound, so a buffer created lazily in the
	// middle of a frame broke whatever was drawn last). So it is filled with no VAO bound, and the VAO comes back after;
	// VertexArray::SetElementBuffer attaches it to its own VAO.
	int boundVertexArray = 0;
	GLCall(glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &boundVertexArray));
	GLCall(glBindVertexArray(0));

	GLCall(glGenBuffers(1, &id));
	GLCall(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, id));
	GLCall(glBufferData(GL_ELEMENT_ARRAY_BUFFER, descriptor.size, descriptor.data, BufferUsageToOpenGL(descriptor.usage)));

	GLCall(glBindVertexArray((unsigned int)boundVertexArray));
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