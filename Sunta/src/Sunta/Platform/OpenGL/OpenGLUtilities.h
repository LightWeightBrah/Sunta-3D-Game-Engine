#pragma once

#include "Core/Assert.h"

namespace Sunta
{

typedef unsigned int GLenum;

enum class BufferUsage;
enum class ShaderDataType;

#ifdef SUNTA_DEBUG	//Sunta:: for safety, to make sure it works in every namespace
	#define GLCall(x) Sunta::GLClearError(); x; SUNTA_ASSERT(Sunta::GLLogCall(#x, __FILE__, __LINE__))
#else
	#define GLCall(x) x
#endif

void GLClearError();
bool GLLogCall(const char* function, const char* file, int line);

GLenum BufferUsageToOpenGL(BufferUsage usage);
GLenum ShaderDataTypeToGLenum(ShaderDataType shaderDataType);

}

