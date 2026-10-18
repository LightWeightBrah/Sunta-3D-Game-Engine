#include "Core/SuntaPreCompiled.h"
#include "EditorShortcuts.h"

#include <imgui/imgui.h>

#include "SceneCommands.h"
#include "Scene/Scene.h"

namespace Sunta
{

namespace
{

bool IsAnyPopupOpen()
{
	return ImGui::IsPopupOpen(nullptr, ImGuiPopupFlags_AnyPopupId | ImGuiPopupFlags_AnyPopupLevel);
}

bool CanRunShortcuts(const Scene& scene)
{
	return !scene.IsPlaying() && !ImGui::GetIO().WantTextInput && !IsAnyPopupOpen();
}

// Ctrl + key, with Shift held or not held exactly as asked (so Ctrl+S and Ctrl+Shift+S never both fire)
bool IsCtrlShortcutPressed(ImGuiKey key, bool withShift)
{
	const ImGuiIO& io = ImGui::GetIO();

	return io.KeyCtrl && !io.KeyAlt && io.KeyShift == withShift && ImGui::IsKeyPressed(key, false);
}

}

void EditorShortcuts::Process(const Scene& scene)
{
	if (!CanRunShortcuts(scene))
		return;

	if (IsCtrlShortcutPressed(ImGuiKey_N, false))
		SceneCommands::New();
	else if (IsCtrlShortcutPressed(ImGuiKey_O, false))
		SceneCommands::Open();
	else if (IsCtrlShortcutPressed(ImGuiKey_S, true))
		SceneCommands::SaveAs();
	else if (IsCtrlShortcutPressed(ImGuiKey_S, false))
		SceneCommands::Save();
}

}
