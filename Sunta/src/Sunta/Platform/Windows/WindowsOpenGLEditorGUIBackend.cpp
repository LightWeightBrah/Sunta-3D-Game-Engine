#include "Core/SuntaPreCompiled.h"
#include "WindowsOpenGLEditorGUIBackend.h"

namespace Sunta
{

WindowsOpenGLEditorGUIBackend::~WindowsOpenGLEditorGUIBackend()
{

}

void WindowsOpenGLEditorGUIBackend::Init(void* window)
{
	implementation.Init(window);
}

void WindowsOpenGLEditorGUIBackend::Shutdown(void* window)
{
	implementation.Shutdown(window);
}

void WindowsOpenGLEditorGUIBackend::NewFrame(void* window)
{
	implementation.NewFrame(window);
}

void WindowsOpenGLEditorGUIBackend::EndFrame(void* window)
{
	implementation.EndFrame(window);
}

void WindowsOpenGLEditorGUIBackend::Render(void* window)
{
	implementation.Render(window);
}

}