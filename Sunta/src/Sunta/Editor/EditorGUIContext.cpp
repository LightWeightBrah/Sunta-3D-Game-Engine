#include "EditorGUIContext.h"
#include <imgui/imgui.h>
#include "../Sunta/Window.h"
#include "EditorGUIBackend.h"
#include "../EventBus.h"
#include "../EventTypes.h"

namespace Sunta
{

std::unique_ptr<EditorGUIBackend> EditorGUIContext::backend;
unsigned int EditorGUIContext::engineModeChangeID;

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

	engineModeChangeID = EventBus::Subscribe<EngineModeChangedEvent>(EditorGUIContext::OnEngineModeChanged);
}

void EditorGUIContext::Shutdown(Window* window)
{
	if (!backend || !window)
		return;

	EventBus::Unsubsribe(engineModeChangeID);
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

void EditorGUIContext::SetInputCapture(bool enabled)
{
	ImGuiIO& io = ImGui::GetIO();

	if (enabled)
	{
		//enable mouse interaction, remove noMouse flag
		io.ConfigFlags &= ~ImGuiConfigFlags_NoMouse;
	}
	else
	{
		//disable interactions, add noMouse flag and set mouse pos outside window
		io.ConfigFlags |= ImGuiConfigFlags_NoMouse;
		io.MousePos = ImVec2(-1.0f, -1.0f);
	}
}

void EditorGUIContext::OnEngineModeChanged(const EngineModeChangedEvent& event)
{
	bool enabled = (event.mode == EngineMode::Editor);
	EditorGUIContext::SetInputCapture(enabled);
}



}