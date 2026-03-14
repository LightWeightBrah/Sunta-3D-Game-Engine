#include "Core/SuntaPreCompiled.h"

#include "OpenGLUtilities.h"
#include "Core/Log.h"

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

}