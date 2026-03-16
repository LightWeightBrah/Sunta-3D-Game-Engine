#include "Core/SuntaPreCompiled.h"

#include "OpenGLUtilities.h"
#include "Core/Log.h"
#include "Renderer/RendererDevice.h"

namespace Sunta
{

void GLClearError()
{
	while (glGetError() != GL_NO_ERROR);
}

bool GLLogCall(const char* function, const char* file, int line)
{
	while (GLenum error = glGetError())
	{
		SUNTA_ENGINE_LOG_ERROR("OPEN_GL ERROR ({}): {} {}: {}", error, function, file, line);
		return false;
	}

	return true;
}

GLenum BufferUsageToOpenGL(BufferUsage usage)
{
	switch (usage)
	{
	case BufferUsage::Static:		return GL_STATIC_DRAW;
	case BufferUsage::Dynamic:		return GL_DYNAMIC_DRAW;
	case BufferUsage::Stream:		return GL_STREAM_DRAW;
	}

	SUNTA_ASSERT(false, "Invalid Buffer Usage!!!")
	return GL_STATIC_DRAW;
}

}