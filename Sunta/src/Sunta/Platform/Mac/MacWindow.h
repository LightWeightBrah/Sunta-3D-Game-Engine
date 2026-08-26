#pragma once

#include <memory>

#include "Core/Window.h"
#include "Platform/GLFW/GLFWWindowImplementation.h"

namespace Sunta
{

class EditorGUIBackend;

class MacWindow : public Window
{
public:
	MacWindow(const std::string& title, int width, int height);
	virtual ~MacWindow();

	virtual void Update() override;
	virtual void EnableMouseCursor(bool enabled) override;
	virtual void SetAsGraphicsTarget() override;
	virtual void Show() override;

	virtual std::unique_ptr<EditorGUIBackend> CreateGUIBackend() override;

	virtual unsigned int GetWidth()  const override  { return implementation.GetWidth();        }
	virtual unsigned int GetHeight() const override  { return implementation.GetHeight();       }
	virtual void* GetNativeWindow()  const override  { return implementation.GetNativeWindow(); }

private:
	GLFWWindowImplementation implementation;

};

}