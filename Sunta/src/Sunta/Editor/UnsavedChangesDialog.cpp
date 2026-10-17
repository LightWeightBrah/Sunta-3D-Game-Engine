#include "Core/SuntaPreCompiled.h"
#include "UnsavedChangesDialog.h"

#include <imgui/imgui.h>

#include "SceneDocument.h"

namespace Sunta
{

namespace
{

constexpr const char* PopupTitle  = "Unsaved changes";
constexpr float       ButtonWidth = 120.0f;

enum class Decision
{
	None,
	Continue,
	Cancel
};

// What to do once the user has answered. Empty when no question is open
std::function<void()> pendingAction;

bool shouldOpenPopup = false;

Decision DrawQuestion()
{
	const ImVec2 buttonSize(ButtonWidth, 0.0f);

	ImGui::Text("The scene \"%s\" has changes that are not saved.", SceneDocument::GetSceneName().c_str());
	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();

	Decision decision = Decision::None;

	// When the user cancels the file dialog of a scene without a file, the question stays open
	if (ImGui::Button("Save", buttonSize) && SceneDocument::Save())
		decision = Decision::Continue;

	ImGui::SameLine();

	if (ImGui::Button("Don't Save", buttonSize))
		decision = Decision::Continue;

	ImGui::SameLine();

	if (ImGui::Button("Cancel", buttonSize))
		decision = Decision::Cancel;

	return decision;
}

void FinishQuestion(Decision decision)
{
	std::function<void()> action = std::move(pendingAction);
	pendingAction = nullptr;

	if (decision == Decision::Continue && action)
		action();
}

}

void UnsavedChangesDialog::RunAfterAsking(std::function<void()> action)
{
	// One question at a time (ESC pressed again while the question is open, for example)
	if (pendingAction)
		return;

	SceneDocument::StopPlayIfRunning();

	if (!SceneDocument::HasUnsavedChanges())
	{
		action();
		return;
	}

	pendingAction   = std::move(action);
	shouldOpenPopup = true;
}

void UnsavedChangesDialog::Draw()
{
	if (shouldOpenPopup)
	{
		ImGui::OpenPopup(PopupTitle);
		shouldOpenPopup = false;
	}

	ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

	if (!ImGui::BeginPopupModal(PopupTitle, nullptr, ImGuiWindowFlags_AlwaysAutoResize))
		return;

	const Decision decision = DrawQuestion();

	if (decision != Decision::None)
		ImGui::CloseCurrentPopup();

	ImGui::EndPopup();

	// After EndPopup, because the action may open a file dialog or close the editor
	if (decision != Decision::None)
		FinishQuestion(decision);
}

}
