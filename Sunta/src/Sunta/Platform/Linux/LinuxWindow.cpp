#include "Core/SuntaPreCompiled.h"
#include "LinuxWindow.h"
#include "LinuxOpenGLEditorGUIBackend.h"

namespace Sunta
{

LinuxWindow::LinuxWindow(const std::string& title, int width, int height)
	: implementation(title, width, height)
{
	
}

LinuxWindow::~LinuxWindow() = default;

void LinuxWindow::Update()
{
	implementation.Update();
}

void LinuxWindow::EnableMouseCursor(bool enabled)
{
	implementation.EnableMouseCursor(enabled);
}

void LinuxWindow::SetAsGraphicsTarget()
{
	implementation.SetAsGraphicsTarget();
}

void LinuxWindow::Show()
{
	implementation.Show();
}

void LinuxWindow::Hide()
{
	implementation.Hide();
}

void LinuxWindow::SetTitle(const std::string& title)
{
	implementation.SetTitle(title);
}

bool LinuxWindow::SetIcon(const std::string& imagePath)
{
	return implementation.SetIcon(imagePath);
}

std::unique_ptr<Sunta::EditorGUIBackend> LinuxWindow::CreateGUIBackend()
{
	return std::make_unique<LinuxOpenGLEditorGUIBackend>();
}

}
