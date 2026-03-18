#include "Sunta/Core/SuntaPreCompiled.h"

#include "OpenGLDevice.h"

namespace Sunta
{

void Sunta::OpenGLDevice::Clear(float r, float g, float b, float a)
{
	GLCall(glClearColor(r, g, b, a));
	GLCall(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));
}

void Sunta::OpenGLDevice::DrawElements(const std::shared_ptr<VertexArray>& VAO)
{
	VAO->Bind();
	unsigned int count = VAO->GetElementBuffer()->GetCount();
	GLCall(glDrawElements(GL_TRIANGLES, count, GL_UNSIGNED_INT, 0));
}

std::shared_ptr<Sunta::VertexBuffer> Sunta::OpenGLDevice::CreateVertexBuffer(const BufferDescriptor& descriptor)
{
	return std::make_shared<OpenGLVertexBuffer>(descriptor);
}

std::shared_ptr<Sunta::ElementBuffer> OpenGLDevice::CreateElementBuffer(const BufferDescriptor& descriptor)
{
	return std::make_shared<OpenGLElementBuffer>(descriptor);
}

std::shared_ptr<Assimp::MD5::VertexArray> OpenGLDevice::CreateVertexArrayBuffer()
{
	return std::make_shared<OpenGLVertexArray>();
}

}