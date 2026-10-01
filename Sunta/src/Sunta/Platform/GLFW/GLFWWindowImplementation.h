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

	unsigned int GetWidth()  const { return width; }
	unsigned int GetHeight() const { return height; }
	void* GetNativeWindow()  const { return window; }

private:
	GLFWwindow* window;
	std::unique_ptr<GraphicsContext> graphicsContext;
	unsigned int engineModeChangedID;

	std::string title;
	int width, height;

	void Init();
	void SetCallbacks();
	void Shutdown();
	void OnEngineModeChanged(const EngineModeChangedEvent& event);

	void SetWindowIcon();
};

}