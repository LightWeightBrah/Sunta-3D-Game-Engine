#include "Core/SuntaPreCompiled.h"
#include "WindowsWindow.h"
#include "WindowsOpenGLEditorGUIBackend.h"

namespace Sunta
{

WindowsWindow::WindowsWindow(const std::string& title, int width, int height)
	: implementation(title, width, height)
{
	
}

WindowsWindow::~WindowsWindow() = default;

void WindowsWindow::Update()
{
	implementation.Update();
}

void WindowsWindow::EnableMouseCursor(bool enabled)
{
	implementation.EnableMouseCursor(enabled);
}

void WindowsWindow::SetAsGraphicsTarget()
{
	implementation.SetAsGraphicsTarget();
}


void WindowsWindow::Show()
{
	implementation.Show();
}

void WindowsWindow::Hide()
{
	implementation.Hide();
}

void WindowsWindow::SetTitle(const std::string& title)
{
	implementation.SetTitle(title);
}

bool WindowsWindow::SetIcon(const std::string& imagePath)
{
	return implementation.SetIcon(imagePath);
}

std::unique_ptr<Sunta::EditorGUIBackend> WindowsWindow::CreateGUIBackend()
{
	return std::make_unique<WindowsOpenGLEditorGUIBackend>();
}

}
