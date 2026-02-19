#pragma once

namespace Sunta
{

class EditorGUIBackend
{
public:
	virtual ~EditorGUIBackend() = default;

	virtual void Init(void* window) = 0;
	virtual void Shutdown(void* window) = 0;
	virtual void NewFrame(void* window) = 0;
	virtual void EndFrame(void* window) = 0;
	virtual void Render(void* window) = 0;
};

}