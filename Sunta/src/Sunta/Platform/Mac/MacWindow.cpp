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

std::unique_ptr<Sunta::EditorGUIBackend> MacWindow::CreateGUIBackend()
{
	return std::make_unique<MacOpenGLEditorGUIBackend>();
}

}
