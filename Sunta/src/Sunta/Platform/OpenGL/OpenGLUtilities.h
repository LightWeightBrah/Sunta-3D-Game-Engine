#pragma once

#include "Core/Assert.h"
#include <glad/glad.h>

namespace Sunta
{

enum class BufferUsage;
enum class ShaderDataType;

#ifdef SUNTA_DEBUG	//Sunta:: for safety, to make sure it works in every namespace
	#define GLCall(x) Sunta::GLClearError(); x; SUNTA_ASSERT(Sunta::GLLogCall(#x, __FILE__, __LINE__))
#else
	#define GLCall(x) x
#endif

void GLClearError();
bool GLLogCall(const char* function, const char* file, int line);

static GLenum BufferUsageToOpenGL(BufferUsage usage);
static GLenum ShaderDataTypeToGLenum(ShaderDataType shaderDataType);

}

