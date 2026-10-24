#include "Core/SuntaPreCompiled.h"
#include "CollisionSettingsWindow.h"

#include <imgui/imgui.h>
#include <imgui/misc/cpp/imgui_stdlib.h>

#include "Core/GameConfig.h"
#include "Physics/CollisionSettings.h"

namespace Sunta
{

namespace
{

constexpr const char* WindowTitle    = "Collision Layers & Matrix";
constexpr float       WindowWidth    = 780.0f;
constexpr float       LayerNameWidth = 220.0f;

const ImVec4 ErrorColor   = ImVec4(1.0f, 0.45f, 0.4f, 1.0f);
const ImVec4 SuccessColor = ImVec4(0.5f, 0.9f, 0.5f, 1.0f);

struct WindowState
{
	bool isOpen = false;

	std::string newLayerName;

	std::string statusMessage;
	ImVec4      statusColor = SuccessColor;
};

WindowState state;

void SetStatus(const std::string& message, const ImVec4& color)
{
	state.statusMessage = message;
	state.statusColor   = color;
}

// Only the collision settings are replaced, the rest of game.config stays as it is
// A broken game.config is never overwritten (it would lose the name and the start scene), the settings still work until the editor closes
void SaveSettings(const CollisionSettings& settings)
{
	GameConfig config;

	if (!GameConfig::TryLoad(GameConfig::DefaultPath, config))
	{
		SetStatus("Couldn't read game.config, the change is not saved (see the log)", ErrorColor);
		return;
	}

	config.collisionSettings = settings;

	if (GameConfig::TrySave(GameConfig::DefaultPath, config))
		SetStatus("Saved to game.config", SuccessColor);
	else
		SetStatus("Couldn't save game.config, see the log", ErrorColor);
}

const char* DescribeNameProblem(CollisionLayerSet::NameProblem problem)
{
	switch (problem)
	{
	case CollisionLayerSet::NameProblem::Empty:                 return "The layer name can't be empty";
	case CollisionLayerSet::NameProblem::SpaceAtTheEdge:        return "The layer name can't start or end with a space";
	case CollisionLayerSet::NameProblem::ContainsPairSeparator: return "The layer name can't contain '+'";
	case CollisionLayerSet::NameProblem::AlreadyUsed:           return "Another layer already has this name";
	case CollisionLayerSet::NameProblem::None:                  break;
	}

	return "";
}

// Returns true when the settings were changed
bool TryRenameLayer(CollisionSettings& settings, CollisionLayer layer, const std::string& newName)
{
	if (newName == settings.GetLayers().GetName(layer))
		return false;

	const CollisionLayerSet::NameProblem problem = settings.GetLayers().CheckName(newName, layer);

	if (problem != CollisionLayerSet::NameProblem::None)
	{
		SetStatus(DescribeNameProblem(problem), ErrorColor);
		return false;
	}

	return settings.TryRenameLayer(layer, newName);
}

// Returns true when the settings were changed
bool TryAddNewLayer(CollisionSettings& settings)
{
	const CollisionLayerSet::NameProblem problem = settings.GetLayers().CheckName(state.newLayerName);

	if (problem != CollisionLayerSet::NameProblem::None)
	{
		SetStatus(DescribeNameProblem(problem), ErrorColor);
		return false;
	}

	if (!settings.TryAddLayer(state.newLayerName))
		return false;

	state.newLayerName.clear();
	return true;
}

void DrawDescription()
{
	ImGui::PushTextWrapPos(0.0f);

	ImGui::TextDisabled("Each collider chooses its layer in the Inspector. Checked in the table = the two layers collide: solid colliders push each other away, "
		"and when one of them is a trigger, the overlap is reported. The first layer is the default one and can't be removed. "
		"A collider that still uses a removed layer collides with nothing until you give it another layer.");

	ImGui::PopTextWrapPos();
	ImGui::Spacing();
}

bool DrawLayerRow(CollisionSettings& settings, CollisionLayer layer)
{
	bool isChanged = false;

	ImGui::PushID(static_cast<int>(layer));

	// The name is applied when the user leaves the field, so a half-typed name (e.g. an empty one) is never rejected on the way
	std::string name = settings.GetLayers().GetName(layer);

	ImGui::SetNextItemWidth(LayerNameWidth);
	ImGui::InputText("##LayerName", &name);

	if (ImGui::IsItemDeactivatedAfterEdit())
		isChanged = TryRenameLayer(settings, layer, name);

	ImGui::SameLine();

	ImGui::BeginDisabled(!settings.GetLayers().CanRemove(layer));

	if (ImGui::Button("Remove"))
	{
		settings.RemoveLayer(layer);
		isChanged = true;
	}

	ImGui::EndDisabled();

	ImGui::PopID();

	return isChanged;
}

bool DrawAddLayerRow(CollisionSettings& settings)
{
	const bool isFull = settings.GetLayers().IsFull();

	ImGui::BeginDisabled(isFull);

	ImGui::SetNextItemWidth(LayerNameWidth);
	const bool isEnterPressed = ImGui::InputTextWithHint("##NewLayerName", "New layer name", &state.newLayerName, ImGuiInputTextFlags_EnterReturnsTrue);

	ImGui::SameLine();
	const bool isAddClicked = ImGui::Button("Add Layer");

	ImGui::EndDisabled();

	if (isFull)
		ImGui::TextDisabled("All %u layers are used", MaxCollisionLayers);

	return (isEnterPressed || isAddClicked) && TryAddNewLayer(settings);
}

bool DrawLayerList(CollisionSettings& settings)
{
	ImGui::TextUnformatted("Layers");
	ImGui::Separator();

	bool isChanged = false;

	for (CollisionLayer layer : settings.GetLayers().GetLayers())
		isChanged = DrawLayerRow(settings, layer) || isChanged;

	return DrawAddLayerRow(settings) || isChanged;
}

// The headers are far from the cells, so the tooltip tells which rule this is
bool DrawMatrixCell(CollisionSettings& settings, CollisionLayer rowLayer, CollisionLayer columnLayer)
{
	ImGui::PushID(static_cast<int>(columnLayer));

	bool canCollide = settings.CanCollide(rowLayer, columnLayer);
	const bool isChanged = ImGui::Checkbox("##CanCollide", &canCollide);

	if (isChanged)
		settings.SetCanCollide(rowLayer, columnLayer, canCollide);

	if (ImGui::IsItemHovered())
		ImGui::SetTooltip("%s + %s", settings.GetLayers().GetName(rowLayer).c_str(), settings.GetLayers().GetName(columnLayer).c_str());

	ImGui::PopID();

	return isChanged;
}

// Only the cells up to the diagonal are drawn, the table is symmetric
bool DrawMatrixRow(CollisionSettings& settings, const std::vector<CollisionLayer>& layers, size_t rowIndex)
{
	ImGui::TableNextRow();
	ImGui::TableSetColumnIndex(0);
	ImGui::TextUnformatted(settings.GetLayers().GetName(layers[rowIndex]).c_str());

	ImGui::PushID(static_cast<int>(layers[rowIndex]));

	bool isChanged = false;

	for (size_t columnIndex = 0; columnIndex <= rowIndex; columnIndex++)
	{
		ImGui::TableSetColumnIndex(static_cast<int>(columnIndex) + 1);
		isChanged = DrawMatrixCell(settings, layers[rowIndex], layers[columnIndex]) || isChanged;
	}

	ImGui::PopID();

	return isChanged;
}

// The checkbox in the row of "Player" and the column of "Environment" is the same rule
// as the one in the row of "Environment" and the column of "Player"
bool DrawMatrixTable(CollisionSettings& settings)
{
	const std::vector<CollisionLayer> layers = settings.GetLayers().GetLayers();

	constexpr ImGuiTableFlags TableFlags = ImGuiTableFlags_Borders | ImGuiTableFlags_SizingFixedFit;

	ImGui::Spacing();
	ImGui::TextUnformatted("Collisions");
	ImGui::Separator();

	if (!ImGui::BeginTable("CollisionMatrixTable", static_cast<int>(layers.size()) + 1, TableFlags))
		return false;

	ImGui::TableSetupColumn("");
	for (CollisionLayer layer : layers)
		ImGui::TableSetupColumn(settings.GetLayers().GetName(layer).c_str());

	ImGui::TableHeadersRow();

	bool isChanged = false;

	for (size_t rowIndex = 0; rowIndex < layers.size(); rowIndex++)
		isChanged = DrawMatrixRow(settings, layers, rowIndex) || isChanged;

	ImGui::EndTable();

	return isChanged;
}

bool DrawResetButton(CollisionSettings& settings)
{
	ImGui::Spacing();

	if (!ImGui::Button("Reset Layers and Rules to Defaults"))
		return false;

	settings.ResetToDefaults();
	return true;
}

void DrawStatusMessage()
{
	if (state.statusMessage.empty())
		return;

	ImGui::Spacing();
	ImGui::TextColored(state.statusColor, "%s", state.statusMessage.c_str());
}

}

void CollisionSettingsWindow::LoadProjectSettings()
{
	GameConfig config;

	if (GameConfig::TryLoad(GameConfig::DefaultPath, config))
		CollisionSettings::SetActive(config.collisionSettings);
}

void CollisionSettingsWindow::DrawMenuItem()
{
	if (ImGui::MenuItem("Collision Layers & Matrix..."))
		state.isOpen = true;
}

void CollisionSettingsWindow::Draw()
{
	if (!state.isOpen)
		return;

	ImGui::SetNextWindowSize(ImVec2(WindowWidth, 0.0f), ImGuiCond_Appearing);

	if (ImGui::Begin(WindowTitle, &state.isOpen))
	{
		DrawDescription();

		CollisionSettings settings = CollisionSettings::GetActive();

		bool isChanged = DrawLayerList(settings);
		isChanged      = DrawMatrixTable(settings) || isChanged;
		isChanged      = DrawResetButton(settings) || isChanged;

		if (isChanged)
		{
			CollisionSettings::SetActive(settings);
			SaveSettings(settings);
		}

		DrawStatusMessage();
	}
	ImGui::End();
}

}
