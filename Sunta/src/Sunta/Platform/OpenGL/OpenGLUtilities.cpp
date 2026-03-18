#include "Core/SuntaPreCompiled.h"

#include "OpenGLUtilities.h"
#include "Core/Log.h"
#include "Renderer/RendererDevice.h"
#include "Renderer/BufferLayout.h"

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

GLenum ShaderDataTypeToOpenGLBaseType(ShaderDataType shaderDataType)
{
	switch (shaderDataType)
	{
	case ShaderDataType::Float:  return GL_FLOAT;
	case ShaderDataType::Float2: return GL_FLOAT;
	case ShaderDataType::Float3: return GL_FLOAT;
	case ShaderDataType::Float4: return GL_FLOAT;

	case ShaderDataType::Mat3:   return GL_FLOAT; // Matricies base type is also GL_FLOAT
	case ShaderDataType::Mat4:   return GL_FLOAT; // Matricies base type is also GL_FLOAT

	case ShaderDataType::Int:    return GL_INT;
	case ShaderDataType::Int2:   return GL_INT;
	case ShaderDataType::Int3:   return GL_INT;
	case ShaderDataType::Int4:   return GL_INT;

	case ShaderDataType::Bool:   return GL_BOOL;
	}

	SUNTA_ASSERT(false, "UNKNOWN SHADER DATA TYPE!!!");
	return 0;
}

}