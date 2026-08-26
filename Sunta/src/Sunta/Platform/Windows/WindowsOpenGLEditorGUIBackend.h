#pragma once
#include "Editor/EditorGUIBackend.h"
#include "Platform/GLFW/GLFWOpenGLEditorGUIBackend.h"

namespace Sunta
{

class WindowsOpenGLEditorGUIBackend : public EditorGUIBackend
{
public:
	virtual ~WindowsOpenGLEditorGUIBackend() override;

	virtual void Init	 (void* window)		 override;
	virtual void Shutdown(void* window)		 override;
	virtual void NewFrame(void* window)		 override;
	virtual void EndFrame(void* window)		 override;
	virtual void Render  (void* window)		 override;

private:
	GLFWOpenGLEditorGUIBackend implementation;
};

}