#pragma once

#include <memory>
#include <string>

struct GLFWwindow;

namespace Sunta
{

struct EngineModeChangedEvent;
class GraphicsContext;

class GLFWWindowImplementation
{
public:
	GLFWWindowImplementation(const std::string& title, int width, int height);
	~GLFWWindowImplementation();

	void Update();
	void EnableMouseCursor(bool enabled);
	void SetAsGraphicsTarget();
	void Show();
	void Hide();
	void SetTitle(const std::string& newTitle);
	bool SetIcon(const std::string& imagePath);
	void SetVSync(bool enabled);
	void SetFullscreen(bool enabled);

	unsigned int GetWidth()  const { return width; }
	unsigned int GetHeight() const { return height; }
	void* GetNativeWindow()  const { return window; }

private:
	GLFWwindow* window;
	std::unique_ptr<GraphicsContext> graphicsContext;
	unsigned int engineModeChangedID;

	std::string title;
	int width, height;

	bool isVSyncEnabled = true;
	bool isFullscreen   = false;

	// The window before it went fullscreen, to bring it back there
	int windowedX = 0, windowedY = 0;
	int windowedWidth = 0, windowedHeight = 0;

	void Init();
	void SetCallbacks();
	void Shutdown();
	void OnEngineModeChanged(const EngineModeChangedEvent& event);

	void SetDefaultIcon();
};

}