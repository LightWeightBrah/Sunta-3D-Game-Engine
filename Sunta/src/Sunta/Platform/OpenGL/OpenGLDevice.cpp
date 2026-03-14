#include "Sunta/Core/SuntaPreCompiled.h"

#include "OpenGLDevice.h"

namespace Sunta
{

void Sunta::OpenGLDevice::Clear(float r, float g, float b, float a)
{
	GLCall(glClearColor(r, g, b, a));
	GLCall(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));

}

void Sunta::OpenGLDevice::DrawElements(unsigned int count)
{

}

std::shared_ptr<Sunta::VertexBuffer> Sunta::OpenGLDevice::CreateVertexBuffer(const BufferDescriptor& descriptor)
{

}

}