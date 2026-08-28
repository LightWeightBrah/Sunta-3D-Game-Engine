#include "Core/SuntaPreCompiled.h"
#include "ScriptComponentDrawer.h"

#include <imgui/imgui.h>
#include <imgui_internal.h>
#include <imgui/misc/cpp/imgui_stdlib.h>

#include "ECS/Component.h"

namespace Sunta
{

void ScriptComponentDrawer::Draw(void* componentData, unsigned int entityID, EntityManager& entityManager)
{
	auto* scriptComponent = static_cast<ScriptComponent*>(componentData);

	ImGui::TextDisabled("Drag & Drop .lua files anywhere below to auto-add script");

	int slotToRemove = -1;

	for (unsigned int i = 0; i < scriptComponent->scripts.size(); i++)
	{
		ImGui::PushID(static_cast<int>(i));

		ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 35.0f);

		if (ImGui::InputText("##Path", &scriptComponent->scripts[i].scriptPath))
			scriptComponent->ReloadScript(scriptComponent->scripts[i], entityID);

		if (ImGui::BeginDragDropTarget())
		{
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("FILE_PATH"))
			{
				std::string droppedPath = static_cast<const char*>(payload->Data);
				if (std::filesystem::path(droppedPath).extension() == ".lua")
				{
					scriptComponent->scripts[i].scriptPath = droppedPath;
					scriptComponent->ReloadScript(scriptComponent->scripts[i], entityID);
				}
			}

			ImGui::EndDragDropTarget();
		}

		ImGui::SameLine();

		if (ImGui::Button("X", ImVec2(24, 0)))
			slotToRemove = static_cast<int>(i);

		ImGui::PopID();

	}

	if (slotToRemove != -1)
		scriptComponent->scripts.erase(scriptComponent->scripts.begin() + slotToRemove);

	ImGui::Spacing();

	if (ImGui::Button("+ Add Empty Slot"))
		scriptComponent->scripts.push_back(ScriptContainer{});

	ImVec2 contentAvailable = ImGui::GetContentRegionAvail();
	if (contentAvailable.y < 30.0f)
		contentAvailable.y = 30.0f;

	ImGui::Dummy(contentAvailable);

	if (ImGui::BeginDragDropTarget())
	{
		if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("FILE_PATH"))
		{
			std::string droppedPath = (const char*)payload->Data;
			if (std::filesystem::path(droppedPath).extension() == ".lua")
			{
				bool exists = false;
				for (const auto& script : scriptComponent->scripts)
				{
					if (script.scriptPath == droppedPath)
					{
						exists = true;
						break;
					}
				}

				if (!exists)
				{
					ScriptContainer newScript;
					newScript.scriptPath = droppedPath;
					scriptComponent->scripts.push_back(newScript);
					scriptComponent->ReloadScript(scriptComponent->scripts.back(), entityID);
				}
			}
		}

		ImGui::EndDragDropTarget();
	}
}

}