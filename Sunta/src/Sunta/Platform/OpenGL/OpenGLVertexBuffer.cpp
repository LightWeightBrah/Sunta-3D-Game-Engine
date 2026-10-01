#include "Core/SuntaPreCompiled.h"
#include "OpenGLVertexBuffer.h"

#include <glad/glad.h>

#include "Renderer/Renderer.h"
#include "Renderer/RendererDevice.h"
#include "OpenGLUtilities.h"
#include "Core/Assert.h"

namespace Sunta
{

OpenGLVertexBuffer::OpenGLVertexBuffer(const BufferDescriptor& descriptor)
	: usage(descriptor.usage)
{
	GLCall(glGenBuffers(1, &id));
	GLCall(glBindBuffer(GL_ARRAY_BUFFER, id));
	GLCall(glBufferData(GL_ARRAY_BUFFER, descriptor.size, descriptor.data, BufferUsageToOpenGL(descriptor.usage)));
}
	
OpenGLVertexBuffer::~OpenGLVertexBuffer()
{
	GLCall(glDeleteBuffers(1, &id));
}
	
void OpenGLVertexBuffer::Bind() const
{
	GLCall(glBindBuffer(GL_ARRAY_BUFFER, id));
}
	
void OpenGLVertexBuffer::Unbind() const
{
	GLCall(glBindBuffer(GL_ARRAY_BUFFER, 0));
}

void OpenGLVertexBuffer::UpdateDynamicData(const void* data, unsigned int size)
{
	SUNTA_ASSERT(usage == BufferUsage::Dynamic, "UpdateDynamicData called on a STATIC Vertex Buffer. VBO Must be with BufferUsage::Dynamic");

	// Writes into the EXISTING GPU allocation instead of creating a new one
	// Safe to call every frame, cause the buffer was created for DYNAMIC usage
	GLCall(glBindBuffer(GL_ARRAY_BUFFER, id));
	GLCall(glBufferSubData(GL_ARRAY_BUFFER, 0, size, data));	
}

}
