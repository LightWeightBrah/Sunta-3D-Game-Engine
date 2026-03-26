#pragma once
#include "Editor/EditorGUIBackend.h"


namespace Sunta
{

class LinuxOpenGLEditorGUIBackend : public EditorGUIBackend
{
public:
	virtual ~LinuxOpenGLEditorGUIBackend() override;

	virtual void Init	 (void* window)		 override;
	virtual void Shutdown(void* window)		 override;
	virtual void NewFrame(void* window)		 override;
	virtual void EndFrame(void* window)		 override;
	virtual void Render  (void* window)		 override;
};

}