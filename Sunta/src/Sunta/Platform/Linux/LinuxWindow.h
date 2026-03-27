#pragma once

#include "Core/Window.h"
#include <memory>

struct GLFWwindow;

namespace Sunta
{

struct EngineModeChangedEvent;
class EditorGUIBackend;
class GraphicsContext;

class LinuxWindow : public Window
{
public:
	LinuxWindow(const std::string& title, int width, int height);
	virtual ~LinuxWindow();

	virtual void Update() override;
	virtual void EnableMouseCursor(bool enabled) override;
	virtual void SetAsGraphicsTarget() override;

	virtual std::unique_ptr<EditorGUIBackend> CreateGUIBackend() override;

	virtual unsigned int GetWidth()  const override { return width;  }
	virtual unsigned int GetHeight() const override { return height; }
	virtual void* GetNativeWindow() const override  { return window; }

private:
	GLFWwindow* window;
	std::unique_ptr<GraphicsContext> graphicsContext;
	unsigned int engineModeChangedID;

	void Init();
	void SetCallbacks();
	void Shutdown();
	void OnEngineModeChanged(const EngineModeChangedEvent& event);
};

}