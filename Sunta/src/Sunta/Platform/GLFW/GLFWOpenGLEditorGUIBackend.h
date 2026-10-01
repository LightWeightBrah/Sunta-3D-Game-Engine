#pragma once

namespace Sunta
{

class GLFWOpenGLEditorGUIBackend
{
public:
	~GLFWOpenGLEditorGUIBackend();
	void Init(void* window);
	void Shutdown(void* window);
	void NewFrame(void* window);
	void EndFrame(void* window);
	void Render(void* window);
};

}