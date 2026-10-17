#pragma once
#include <memory>
#include <string>

namespace Sunta
{

class EditorGUIBackend;

class Window
{

public:
	//DESTRUCTOR: makes sure every class that dervies from Window 
	//e.g WindowsWindow is correctly deleted after deleting base Window
	virtual ~Window() = default;

	virtual void Update() = 0;
	virtual void EnableMouseCursor(bool enabled) = 0;
	virtual void SetAsGraphicsTarget() = 0;
	virtual void Show() = 0;

	// Sets the icon of the window (title bar, taskbar) from an image file (PNG)
	// Returns false and keeps the current icon if the image can't be loaded
	virtual bool SetIcon(const std::string& imagePath) = 0;

	virtual std::unique_ptr<EditorGUIBackend> CreateGUIBackend() = 0;

	virtual unsigned int GetWidth() const = 0;
	virtual unsigned int GetHeight() const = 0;
	virtual void* GetNativeWindow() const = 0;

	static std::unique_ptr<Window> CreateWindow(const std::string& title, int width, int height);

};

}
