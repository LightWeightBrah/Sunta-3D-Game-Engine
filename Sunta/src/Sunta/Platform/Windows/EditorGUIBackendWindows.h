#pragma once
#include "../../Editor/EditorGUIBackend.h"


namespace Sunta
{

class EditorGUIBackendWindows : public EditorGUIBackend
{
public:
	virtual ~EditorGUIBackendWindows() override;

	virtual void Init	 (void* window)		 override;
	virtual void Shutdown(void* window)		 override;
	virtual void NewFrame(void* window)		 override;
	virtual void EndFrame(void* window)		 override;
	virtual void Render  (void* window)		 override;
};

}