#include "Core/SuntaPreCompiled.h"
#include "EditorGUI.h"

#include <imgui/imgui.h>
#include <imgui_internal.h>
#include <imgui/misc/cpp/imgui_stdlib.h>

#include <algorithm>

#include "ECS/EntityManager.h"
#include "ECS/ComponentLayout.h"
#include "ECS/Component.h"
#include "Core/ResourceManager.h"
#include "Renderer/Texture.h"

namespace Sunta
{

void EditorGUI::Begin(const std::string& name)
{
	ImGui::Begin(name.c_str());
}

void EditorGUI::End()
{
	ImGui::End();
}

bool EditorGUI::BeginGroup(const std::string& label)
{
	return ImGui::CollapsingHeader(label.c_str(), ImGuiTreeNodeFlags_DefaultOpen);
}

void EditorGUI::EndGroup()
{

}

bool EditorGUI::PropertyFloat3(const std::string& label, glm::vec3& value)
{
	return ImGui::DragFloat3(label.c_str(), &value.x, 0.1f);
}

bool EditorGUI::PropertyColor(const std::string& label, glm::vec4& color)
{
	return ImGui::ColorEdit4(label.c_str(), &color.r);
}

bool EditorGUI::PropertyBool(const std::string& label, bool& value)
{
	return ImGui::Checkbox(label.c_str(), &value);
}

void EditorGUI::Text(const std::string& text)
{
	ImGui::Text(text.c_str());
}

bool EditorGUI::Button(const std::string& label)
{
	return ImGui::Button(label.c_str());
}

void EditorGUI::DrawHierarchy(EntityManager& entityManager)
{
	unsigned int totalEntities = entityManager.GetEntityCount();

	for (unsigned int entityID = 0; entityID < totalEntities; entityID++)
	{
		std::string label = ("Entity " + std::to_string(entityID));

		if (auto* tag = entityManager.GetComponent<TagComponent>(entityID))
		{
			if (!tag->name.empty())
				label = tag->name;
		}

		// ### is special imgui separator, everything after this is seen via imGUI
		// as a permament unique ID (so we won't see ###EntityID-, but it works for 
		// changing names in inspector)
		std::string imguiLabel = label + "###EntityID-" + std::to_string(entityID);

		ImGuiTreeNodeFlags flags = (selectedEntity == static_cast<int>(entityID)) ? ImGuiTreeNodeFlags_Selected : 0;

		flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_NoTreePushOnOpen;

		ImGui::TreeNodeEx(imguiLabel.c_str(), flags);

		if (ImGui::IsItemClicked())
		{
			selectedEntity = static_cast<int>(entityID);
		}
		
	}
}

void EditorGUI::DrawInspector(EntityManager& entityManager)
{
	if (selectedEntity == -1)
	{
		return;
	}

	ImGui::Text("Selected Entity ID: %d", selectedEntity);
	ImGui::Separator();

	// unique ID for ImGUI, so objects with same name can be in tree hierarchy
	// PushID makes everything drawn below belongs to that unique ID
	ImGui::PushID(selectedEntity);

	DrawEntityComponentList(selectedEntity, entityManager);

	// here we stop using that uniqueID for ImGUI and we go back to defualt mode
	ImGui::PopID();

}

void EditorGUI::DrawFileBrowser()
{
	if (currentDirectory.empty())
	{
		ImGui::Text("Select file:");
	}
	else
	{
		if (ImGui::Button("<--- Back"))
		{
			currentDirectory = currentDirectory.parent_path();
		}

		ImGui::SameLine();
		ImGui::Text("Path: %s", currentDirectory.string().c_str());
	}

	ImGui::Separator();

	float cellSize = 90.0f;
	float panelWidth = ImGui::GetContentRegionAvail().x;
	int padding	= 15;
	int columns = static_cast<int>(panelWidth / cellSize);
	if (columns < 1)
		columns = 1;

	ImGui::Columns(columns, nullptr, false);

	// we are in root
	if (currentDirectory.empty())
	{
		// Sunta Engine Folder
		if (ImGui::Button("[ENGINE]", ImVec2(cellSize - padding, cellSize - padding)))
			currentDirectory = "res/Sunta";

		ImGui::Text("Sunta res");
		ImGui::NextColumn();

		// Game Folder
		if(ImGui::Button("[GAME]", ImVec2(cellSize - padding, cellSize - padding)))
			currentDirectory =  "res/Game";

		ImGui::Text("Game res");
	}
	else // if we are inside other folder
	{
		if (!std::filesystem::exists(currentDirectory))
		{
			ImGui::TextColored(ImVec4(1, 0, 0, 1), "Missing res folder");
			return;
		}

		for (auto& entry : std::filesystem::directory_iterator(currentDirectory))
		{
			const auto& path = entry.path();
			std::string filename = path.filename().string();

			// ignore files like .gitkeep
			if (filename[0] == '.')
				continue;

			ImGui::PushID(filename.c_str());

			bool isDirectory = entry.is_directory();

			std::string iconKey = GetIconKeyForPath(path, isDirectory);
			auto icon = ResourceManager::GetEditorIcon(iconKey);
			ImTextureID textureID = (ImTextureID)(uintptr_t)icon->GetID();

			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));

			if (ImGui::ImageButton(("##" + filename).c_str(), textureID, ImVec2(cellSize - padding, cellSize - padding)))
			{
				if (isDirectory) 
					currentDirectory /= path.filename();
				else 
					selectedFile = path;
			}

			ImGui::PopStyleColor();

			ImGui::PushTextWrapPos(ImGui::GetCursorPos().x + cellSize - padding);
			ImGui::Text("%s", filename.c_str());
			ImGui::PopTextWrapPos();

			ImGui::NextColumn();
			ImGui::PopID();
		}
	}

	ImGui::Columns(1); // column reset
}

std::string EditorGUI::GetIconKeyForPath(const std::filesystem::path& path, bool isDirectory)
{
	if (isDirectory)
	{
		static const std::unordered_map<std::string, std::string> folderIcons = 
		{
			{ "scripts",		"cpp_folder" },			{ "src ",		"cpp_folder" },			{ "cpp ",	"cpp_folder" },
			{ "models",			"3d_model_folder" },	{ "meshes ",	"3d_model_folder" },
			{ "shaders",		"shader_folder" },
			{ "textures",		"image_folder" },		{ "sprites",	"image_folder" },		{ "images", "image_folder" },
			{ "audio",			"audio_folder" },		{ "sounds",		"audio_folder" },		{ "sfx",	"audio_folder" } 
		};

		std::string name = path.filename().string();
		std::transform(name.begin(), name.end(), name.begin(), ::tolower);

		auto it = folderIcons.find(name);
		return (it != folderIcons.end()) ? it->second : "defualt_folder";
	}
	else
	{
		static const std::unordered_map<std::string, std::string> fileIcons =
		{
			{".shader",		"shader_file"},
			{".png",		"image_file"},		{".jpg", "image_file"},		{".jpeg", "image_file"},
			{".obj",		"3d_model_file"},	{".fbx", "3d_model_file"},
			{".cpp",		"cpp_file"},		{".h", "cpp_file"},
			{".wav",		"audio_file"},		{".ogg", "audio_file"}
		};

		std::string extension = path.extension().string();
		std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);

		auto it = fileIcons.find(extension);
		return (it != fileIcons.end()) ? it->second : "defualt_file";

	}
}

void EditorGUI::DrawEntityComponentList(unsigned int entityID, EntityManager& entityManager)
{
	auto& inspectableMap = entityManager.GetInspectableMap();

	// Draw tag component first
	size_t tagHash = typeid(TagComponent).hash_code();
	auto tagIterator = inspectableMap.find(tagHash);
	if (tagIterator != inspectableMap.end() && tagIterator->second->Contains(entityID))
	{
		const auto* componentType = InspectorComponentRegistry::GetComponentTypeByHash(tagHash);
		if (componentType)
			DrawSingleComponent(entityID, componentType, tagIterator->second);
	}

	// Draw transform component second
	size_t transformHash = typeid(TransformComponent).hash_code();
	auto transformIterator = inspectableMap.find(transformHash);
	if (transformIterator != inspectableMap.end() && transformIterator->second->Contains(entityID))
	{
		const auto* componentType = InspectorComponentRegistry::GetComponentTypeByHash(transformHash);
		if (componentType)
			DrawSingleComponent(entityID, componentType, transformIterator->second);
	}

	// Draw rest of the components randomly
	for (auto const& [typeHash, storage] : inspectableMap)
	{
		// Don't draw again tag nor transform components, since they are already drawn
		if (typeHash == tagHash || typeHash == transformHash)
			continue;

		if (!storage->Contains(entityID))
			continue;

		const auto* componentType = InspectorComponentRegistry::GetComponentTypeByHash(typeHash);

		if (!componentType)
			continue;

		DrawSingleComponent(entityID, componentType, storage);

	}
}

void EditorGUI::DrawSingleComponent(unsigned int entityID, const ComponentType* componentType, IInspectableStorage* storage)
{
	if (!ImGui::CollapsingHeader(componentType->name.c_str()))
		return;

	void* componentData = storage->GetEntityComponentData(entityID);
	bool anyPropertyChanged = false;

	for (const auto& property : componentType->properties)
	{
		void* propertyData = (char*)componentData + property.byteOffset;

		if (DrawPropertyWidget(property, propertyData))
			anyPropertyChanged = true;
	}

	if (anyPropertyChanged && componentType->onChanged)
		componentType->onChanged(componentData);
}

bool EditorGUI::DrawPropertyWidget(const PropertyDefinition& property, void* propertyData)
{
	bool changed = false;

	switch (property.dataType)
	{
	case PropertyDataType::Int:	
		changed = ImGui::DragInt(property.label.c_str(), (int*)propertyData); 
		break;
	case PropertyDataType::Bool:
		changed = ImGui::Checkbox(property.label.c_str(), (bool*)propertyData);
		break;
	case PropertyDataType::Float:
		changed = ImGui::DragFloat(property.label.c_str(), (float*)propertyData, 0.1f);
		break;
	case PropertyDataType::Float3:
		if (property.HasRange())
			changed = ImGui::SliderFloat3(property.label.c_str(), (float*)propertyData, property.minValue, property.maxValue);
		else
			changed = ImGui::DragFloat3(property.label.c_str(), (float*)propertyData, 0.1f);
		break;
	case PropertyDataType::Color:
		changed = ImGui::ColorEdit3(property.label.c_str(), (float*)propertyData);
		break;
	case PropertyDataType::String:
		changed = ImGui::InputText(property.label.c_str(), (std::string*)propertyData);
		break;
	}
	
	return changed;
}

void EditorGUI::ClearFocus()
{
	if (!ImGui::GetCurrentContext())
		return;
	
	//Removes foucs from the active widget (slider, button, color etc.)
	ImGui::ClearActiveID();

	//Deselects the window
	ImGui::SetWindowFocus(nullptr);
}




}