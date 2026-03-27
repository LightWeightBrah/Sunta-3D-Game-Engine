#pragma once
#include "Editor/EditorGUIBackend.h"


namespace Sunta
{

class MacOpenGLEditorGUIBackend : public EditorGUIBackend
{
public:
	virtual ~MacOpenGLEditorGUIBackend() override;

	virtual void Init	 (void* window)		 override;
	virtual void Shutdown(void* window)		 override;
	virtual void NewFrame(void* window)		 override;
	virtual void EndFrame(void* window)		 override;
	virtual void Render  (void* window)		 override;
};

}