#include "Core/SuntaPreCompiled.h"
#include "LinuxOpenGLEditorGUIBackend.h"

namespace Sunta
{

LinuxOpenGLEditorGUIBackend::~LinuxOpenGLEditorGUIBackend()
{

}

void LinuxOpenGLEditorGUIBackend::Init(void* window)
{
	implementation.Init(window);
}

void LinuxOpenGLEditorGUIBackend::Shutdown(void* window)
{
	implementation.Shutdown(window);
}

void LinuxOpenGLEditorGUIBackend::NewFrame(void* window)
{
	implementation.NewFrame(window);
}

void LinuxOpenGLEditorGUIBackend::EndFrame(void* window)
{
	implementation.EndFrame(window);
}

void LinuxOpenGLEditorGUIBackend::Render(void* window)
{
	implementation.Render(window);
}

}