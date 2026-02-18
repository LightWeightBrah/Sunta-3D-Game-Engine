#pragma once

#include "../../Window.h"

namespace Sunta
{

class WindowsWindow : public Window
{
public:
	WindowsWindow(int width, int height, const std::string& title);
	virtual ~WindowsWindow();

	virtual void Update() override;
	virtual unsigned int GetWidth()  const override { return width;  }
	virtual unsigned int GetHeight() const override { return height; }

private:
	GLFWwindow* window;

	void Init();
	void SetCallbacks();
	void Shutdown();
};

}