#include "EditorGUIContext.h"
#include <imgui/imgui.h>
#include "../Sunta/Window.h"
#include "EditorGUIBackend.h"

namespace Sunta
{

std::unique_ptr<EditorGUIBackend> EditorGUIContext::backend;

EditorGUIContext::~EditorGUIContext() = default;

void EditorGUIContext::Init(Window* window)
{
	if (!window)
		return;
	
	backend = window->CreateGUIBackend();

	if (!backend)
		return;

	backend->Init(window->GetNativeWindow());
	window->SetAsGraphicsTarget();
}

void EditorGUIContext::Shutdown(Window* window)
{
	if (!backend || !window)
		return;

	backend->Shutdown(window->GetNativeWindow());
	backend.reset();
}

void EditorGUIContext::NewFrame(Window* window)
{
	if (!backend || !window)
		return;

	backend->NewFrame(window->GetNativeWindow());
}

void EditorGUIContext::EndFrame(Window* window)
{
	if (!backend || !window)
		return;

	backend->EndFrame(window->GetNativeWindow());

	if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		window->SetAsGraphicsTarget();
}

}