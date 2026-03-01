#pragma once

#include "Window.h"
#include <memory>

namespace Sunta
{

struct EngineModeChangedEvent;
class EditorGUIBackend;

class WindowsWindow : public Window
{
public:
	WindowsWindow(int width, int height, const std::string& title);
	virtual ~WindowsWindow();

	virtual void Update() override;
	virtual void EnableMouseCursor(bool enabled) override;
	virtual void SetAsGraphicsTarget() override;

	virtual std::unique_ptr<EditorGUIBackend> CreateGUIBackend() override;

	virtual unsigned int GetWidth()  const override { return width;  }
	virtual unsigned int GetHeight() const override { return height; }
	virtual void* GetNativeWindow() const override  { return window; }

private:
	GLFWwindow* window;
	unsigned int engineModeChangedID;

	void Init();
	void SetCallbacks();
	void Shutdown();
	void OnEngineModeChanged(const EngineModeChangedEvent& event);
};

}