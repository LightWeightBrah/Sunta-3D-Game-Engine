#include "Core/SuntaPreCompiled.h"
#include "MacOpenGLEditorGUIBackend.h"

namespace Sunta
{

MacOpenGLEditorGUIBackend::~MacOpenGLEditorGUIBackend()
{

}

void MacOpenGLEditorGUIBackend::Init(void* window)
{
	implementation.Init(window);
}

void MacOpenGLEditorGUIBackend::Shutdown(void* window)
{
	implementation.Shutdown(window);
}

void MacOpenGLEditorGUIBackend::NewFrame(void* window)
{
	implementation.NewFrame(window);
}

void MacOpenGLEditorGUIBackend::EndFrame(void* window)
{
	implementation.EndFrame(window);
}

void MacOpenGLEditorGUIBackend::Render(void* window)
{
	implementation.Render(window);
}

}