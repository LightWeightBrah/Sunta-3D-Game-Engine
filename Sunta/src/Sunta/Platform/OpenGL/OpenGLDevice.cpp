#include "Sunta/Core/SuntaPreCompiled.h"
#include "OpenGLDevice.h"

#include <glad/glad.h>

#include "OpenGLTexture.h"
#include "OpenGLShader.h"
#include "OpenGLVertexArray.h"
#include "OpenGLVertexBuffer.h"
#include "OpenGLElementBuffer.h"
#include "OpenGLUtilities.h"

namespace Sunta
{

void OpenGLDevice::Clear(float r, float g, float b, float a)
{
	GLCall(glClearColor(r, g, b, a));
	GLCall(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));
}

void OpenGLDevice::SetViewport(int x, int y, int width, int height)
{
	GLCall(glViewport(x, y, width, height));
}

void OpenGLDevice::DrawElements(const std::shared_ptr<VertexArray>& VAO)
{
	VAO->Bind();
	unsigned int count = VAO->GetElementBuffer()->GetCount();
	GLCall(glDrawElements(GL_TRIANGLES, count, GL_UNSIGNED_INT, 0));
}

std::shared_ptr<VertexBuffer> OpenGLDevice::CreateVertexBuffer(const BufferDescriptor& descriptor)
{
	return std::make_shared<OpenGLVertexBuffer>(descriptor);
}

std::shared_ptr<ElementBuffer> OpenGLDevice::CreateElementBuffer(const BufferDescriptor& descriptor)
{
	return std::make_shared<OpenGLElementBuffer>(descriptor);
}

std::shared_ptr<VertexArray> OpenGLDevice::CreateVertexArrayBuffer()
{
	return std::make_shared<OpenGLVertexArray>();
}

std::shared_ptr<Texture> OpenGLDevice::CreateTexture(const std::string& filepath)
{
	return std::make_shared<OpenGLTexture>(filepath);
}

std::shared_ptr<Shader> OpenGLDevice::CreateShader(const std::string& filepath)
{
	return std::make_shared<OpenGLShader>(filepath);
}

}