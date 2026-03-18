#include "Core/SuntaPreCompiled.h"

#include "OpenGLVertexBuffer.h"
#include "Renderer/Renderer.h"
#include "Renderer/RendererDevice.h"
#include "OpenGLUtilities.h"

namespace Sunta
{
	OpenGLVertexBuffer::OpenGLVertexBuffer(const BufferDescriptor& descriptor)
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
}
