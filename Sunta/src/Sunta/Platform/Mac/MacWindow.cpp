#include "Core/SuntaPreCompiled.h"
#include "MacWindow.h"
#include "MacOpenGLEditorGUIBackend.h"

namespace Sunta
{

MacWindow::MacWindow(const std::string& title, int width, int height)
	: implementation(title, width, height)
{
	
}

MacWindow::~MacWindow() = default;

void MacWindow::Update()
{
	implementation.Update();
}

void MacWindow::EnableMouseCursor(bool enabled)
{
	implementation.EnableMouseCursor(enabled);
}

void MacWindow::SetAsGraphicsTarget()
{
	implementation.SetAsGraphicsTarget();
}

void MacWindow::Show()
{
	implementation.Show();
}

void MacWindow::Hide()
{
	implementation.Hide();
}

void MacWindow::SetTitle(const std::string& title)
{
	implementation.SetTitle(title);
}

bool MacWindow::SetIcon(const std::string& imagePath)
{
	return implementation.SetIcon(imagePath);
}

void MacWindow::SetVSync(bool enabled)
{
	implementation.SetVSync(enabled);
}

void MacWindow::SetFullscreen(bool enabled)
{
	implementation.SetFullscreen(enabled);
}

std::unique_ptr<Sunta::EditorGUIBackend> MacWindow::CreateGUIBackend()
{
	return std::make_unique<MacOpenGLEditorGUIBackend>();
}

}
