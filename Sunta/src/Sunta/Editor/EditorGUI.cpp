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
#include "Renderer/Material.h"
#include "Scene/EntityFactory.h"
#include "Scene/Scene.h"
#include "Core/Log.h"
#include "Core/Platform.h"
#include "Core/EngineAssets.h"
#include "Serialization/MaterialSerializer.h"

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

void EditorGUI::DrawToolbar(Scene& scene, RendererDevice& rendererDevice, float toolbarHeight)
{
	ImGuiViewport* viewport = ImGui::GetMainViewport();

	ImGui::SetNextWindowPos(ImVec2(viewport->Pos.x, viewport->Pos.y));
	ImGui::SetNextWindowSize(ImVec2(viewport->Size.x, toolbarHeight));

	float borderThickness = 2.0f;

	ImGuiWindowFlags toolbarFlags =
		  ImGuiWindowFlags_NoDecoration
		| ImGuiWindowFlags_NoScrollbar
		| ImGuiWindowFlags_NoScrollWithMouse
		| ImGuiWindowFlags_NoDocking
		| ImGuiWindowFlags_NoMove
		| ImGuiWindowFlags_NoResize
		| ImGuiWindowFlags_NoSavedSettings;

	float framePadding = 2.0f;

	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(6.0f, 3.0f));
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(framePadding, framePadding));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

	ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0));
	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.3f, 0.3f, 0.5f));

	if (ImGui::Begin("##Toolbar", nullptr, toolbarFlags))
	{
		DrawWindowBackground("file_browser_bg", glm::vec4(0.5f, 0.45f, 0.4f, 1.0f));
		//DrawWindowBackground("file_browser_bg", glm::vec4(0.3f, 0.25f, 0.2f, 1.0f));

		float availableHeight = ImGui::GetContentRegionAvail().y - borderThickness;
		float iconSize = availableHeight - (framePadding * 2);

		auto DrawToolbarButton = [&](const char* iconKey, const char* id, const char* tooltip, auto action)
		{
			auto icon = ResourceManager::GetEditorIcon(iconKey);
			if (icon)
			{
				ImTextureID textureID = (ImTextureID)(uintptr_t)icon->GetID();
				if (ImGui::ImageButton(id, textureID, ImVec2(iconSize, iconSize)))
					action();

				if (ImGui::IsItemHovered())
					ImGui::SetTooltip("%s", tooltip);

				ImGui::SameLine();
			}
		};

		using namespace Sunta::EngineAssets;

		DrawToolbarButton(Icons::Cube, "##CreateCube", "Create Cube Entity", [&]() 
			{
				EntityFactory::CreateCube(scene, rendererDevice, glm::vec3(0.0f, 2.0f, 0.0f), "Cube");
			});

		DrawToolbarButton(Icons::Pyramid, "##CreatePyramid", "Create Pyramid Entity", [&]()
			{
				EntityFactory::CreatePyramid(scene, rendererDevice, glm::vec3(0.0f, 2.0f, 0.0f), "Pyramid");
			});

		DrawToolbarButton(Icons::Cone, "##CreateCone", "Create Cone Entity", [&]()
			{
				EntityFactory::CreateCone(scene, rendererDevice, glm::vec3(0.0f, 2.0f, 0.0f), "Cone");
			});

		DrawToolbarButton(Icons::Sphere, "##CreateSphere", "Create Sphere Entity", [&]()
			{
				EntityFactory::CreateSphere(scene, rendererDevice, glm::vec3(0.0f, 2.0f, 0.0f), "Sphere");
			});

		DrawToolbarButton(Icons::Capsule, "##CreateCapsule", "Create Capsule Entity", [&]()
			{
				EntityFactory::CreateCapsule(scene, rendererDevice, glm::vec3(0.0f, 2.0f, 0.0f), "Capsule");
			});

		DrawToolbarButton(Icons::DirectionalLight, "##CreateDirectionalLight", "Create Directional Light", [&]()
			{
				EntityFactory::CreateDirectionalLight(scene, rendererDevice, glm::vec3(0.0f, 2.0f, 0.0f), "Directional Light");
			});
		
		DrawToolbarButton(Icons::PointLight, "##CreatePointLight", "Create Point Light", [&]()
			{
				EntityFactory::CreatePointLight(scene, rendererDevice, glm::vec3(0.0f, 2.0f, 0.0f), "Point Light");
			});

		DrawToolbarButton(Icons::Spotlight, "##CreateSpotLight", "Create Spot Light", [&]()
			{
				EntityFactory::CreateSpotLight(scene, rendererDevice, glm::vec3(0.0f, 2.0f, 0.0f), "Spot Light");
			});

		// Draw bottom line of toolbar

		ImVec2 pos = ImGui::GetWindowPos();
		ImVec2 size = ImGui::GetWindowSize();
		ImDrawList* drawList = ImGui::GetWindowDrawList();

		ImU32 borderColor = ImGui::GetColorU32(ImGuiCol_Border);

		drawList->AddRectFilled(
			ImVec2(pos.x,          pos.y + size.y - borderThickness),
			ImVec2(pos.x + size.x, pos.y + size.y),
			borderColor);

		ImGui::End();
	}

	ImGui::PopStyleColor(3);
	ImGui::PopStyleVar(3);
}

void EditorGUI::DrawHierarchy(EntityManager& entityManager)
{
	unsigned int totalEntities = entityManager.GetEntityCount();

	for (unsigned int entityID = 0; entityID < totalEntities; entityID++)
	{
		std::string label = ("Entity " + std::to_string(entityID));

		if (auto* tag = entityManager.GetComponent<TagComponent>(entityID))
			if (!tag->name.empty())
				label = tag->name;

		// ### is special imgui separator, everything after this is seen via imGUI
		// as a permament unique ID (so we won't see ###EntityID-, but it works for 
		// changing names in inspector)
		std::string uniqueID = "###EntityID-" + std::to_string(entityID);

		if (entityToRename == static_cast<int>(entityID))
		{
			ImGui::SetNextItemWidth(-1.0f);
			ImGui::SetKeyboardFocusHere();

			if (ImGui::InputText(uniqueID.c_str(), entityNameBuffer, IM_ARRAYSIZE(entityNameBuffer), ImGuiInputTextFlags_EnterReturnsTrue))
			{
				if (auto* tag = entityManager.GetComponent<TagComponent>(entityID))
					tag->name = entityNameBuffer;

				entityToRename = -1;
			}

			if (ImGui::IsItemDeactivated() && !ImGui::IsKeyDown(ImGuiKey_Enter))
				entityToRename = -1;
		}
		else
		{
			ImGuiTreeNodeFlags flags = (selectedEntity == static_cast<int>(entityID)) ? ImGuiTreeNodeFlags_Selected : 0;
			flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_NoTreePushOnOpen;
			ImGui::TreeNodeEx((label + uniqueID).c_str(), flags);

			if (ImGui::IsItemFocused() && ImGui::IsKeyPressed(ImGuiKey_F2))
			{
				entityToRename = entityID;
				strncpy(entityNameBuffer, label.c_str(), sizeof(entityNameBuffer));
			}
		}

		ImGui::PushID(static_cast<int>(entityID));
		if (ImGui::BeginPopupContextItem("EntityMenu"))
		{
			selectedEntity = static_cast<int>(entityID);
			selectedFile = "";

			if (ImGui::MenuItem("Rename"))
			{
				entityToRename = entityID;
				strncpy(entityNameBuffer, label.c_str(), sizeof(entityNameBuffer));
			}

			if (ImGui::MenuItem("Delete"))
			{
				// TODO: add implementation for Destroy Entity in entity Manager
				//entityManager.DestroyEntity(entityID);
				ImGui::CloseCurrentPopup();
			}

			if (ImGui::MenuItem("Reset Transform"))
			{
				if (auto* transform = entityManager.GetComponent<TransformComponent>(entityID))
				{
					transform->position = glm::vec3(0.0f);
					transform->rotation = glm::vec3(0.0f);
					transform->scale    = glm::vec3(1.0f);
				}

				ImGui::CloseCurrentPopup();
			}

			ImGui::EndPopup();
		}

		ImGui::PopID();

		if (ImGui::IsItemClicked())
		{
			selectedEntity = static_cast<int>(entityID);
			selectedFile = "";
		}

	}

	if (IsClickingEmptySpace())
	{
		selectedEntity = -1;
		selectedFile = "";
	}
}

void EditorGUI::DrawInspector(EntityManager& entityManager)
{
	if (!selectedFile.empty() && selectedFile.extension() == ".material")
	{
		ImGui::Text("Material Asset: %s", selectedFile.filename().string().c_str());
		ImGui::Separator();

		if (lastSelectedFile != selectedFile)
		{
			lastSelectedFile = selectedFile;

			currentMaterial = ResourceManager::LoadMaterialFromFile(selectedFile.string());
		}

		if (currentMaterial)
		{
			auto& data = currentMaterial->GetData();
			bool changed = false;

			changed |= ImGui::ColorEdit3("Ambient",  &data.ambientColor.r);
			changed |= ImGui::ColorEdit3("Diffuse",  &data.diffuseColor.r);
			changed |= ImGui::ColorEdit3("Specular", &data.specularColor.r);
			changed |= ImGui::DragFloat("Shininess", &data.shininess, 0.5f, 1.0f, 256.0f);

			if (changed)
				MaterialSerializer::Serialize(selectedFile.string(), currentMaterial);
		}

		return;
		
	}
	else
	{
		lastSelectedFile.clear();
		currentMaterial = nullptr;
	}


	if (IsClickingEmptySpace())
	{
		selectedFile = "";
	}

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
			selectedFile = "";
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
		{
			currentDirectory = "res/Sunta";
			selectedFile = "";
		}

		ImGui::Text("Sunta res");
		ImGui::NextColumn();

		// Game Folder
		if (ImGui::Button("[GAME]", ImVec2(cellSize - padding, cellSize - padding)))
		{
			currentDirectory =  "res/Game";
			selectedFile = "";
		}

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

			ImVec2 size = ImVec2(cellSize - padding, cellSize - padding);
			ImVec2 pos = ImGui::GetCursorScreenPos();
			ImDrawList* drawList = ImGui::GetWindowDrawList();

			ImU32 borderColor = ImGui::GetColorU32(ImGuiCol_Border);
			ImU32 activeColor = ImGui::GetColorU32(ImGuiCol_ButtonActive);
			ImU32 hoverColor = ImGui::GetColorU32(ImGuiCol_ButtonHovered);

			// Invisible Button works as hitbox 
			ImGui::InvisibleButton(("##" + filename).c_str(), size);

			bool isSelected = (selectedFile == path);
			bool isHovered = ImGui::IsItemHovered();

			if (ImGui::IsItemClicked())
			{
				selectedFile = path;
				selectedEntity = -1;
			}

			if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
			{
				std::string pathString = path.string();
				ImGui::SetDragDropPayload("FILE_PATH", pathString.c_str(), pathString.size() + 1);

				ImGui::Text("Dragging: %s", path.filename().string().c_str());

				ImGui::EndDragDropSource();
			}

			if (isSelected)
			{
				drawList->AddRectFilled(pos, ImVec2(pos.x + size.x, pos.y + size.y), activeColor);
			}
			else if (isHovered)
			{
				drawList->AddRectFilled(pos, ImVec2(pos.x + size.x, pos.y + size.y), hoverColor);
			}

			drawList->AddRect(pos, ImVec2(pos.x + size.x, pos.y + size.y), borderColor);

			float margin = 4.0f;
			ImVec2 iconPos0 = ImVec2(pos.x + margin, pos.y + margin);
			ImVec2 iconPos1 = ImVec2(pos.x + size.x - margin, pos.y + size.y - margin);

			std::string iconKey = GetIconKeyForPath(path, isDirectory);
			auto icon = ResourceManager::GetEditorIcon(iconKey);
			if (icon)
			{
				ImTextureID textureID = (ImTextureID)(uintptr_t)icon->GetID();
				drawList->AddImage(textureID, iconPos0, iconPos1);
			}

			if (ImGui::IsItemHovered())
			{
				if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
				{
					if (isDirectory)
					{
						currentDirectory /= path.filename();
						selectedFile = "";
					}
				}
			}

			if (isSelected && ImGui::IsKeyPressed(ImGuiKey_F2))
			{
				fileToRename = path;
				strncpy(fileRenameBuffer, filename.c_str(), sizeof(fileRenameBuffer));
			}

			if (ImGui::BeginPopupContextItem("FileMenu"))
			{
				selectedFile = path;

				if (ImGui::MenuItem("Rename"))
				{
					fileToRename = path;
					strncpy(fileRenameBuffer, filename.c_str(), sizeof(fileRenameBuffer));
				}

				if (ImGui::MenuItem("Delete"))
				{
					std::filesystem::remove(path);
					ImGui::CloseCurrentPopup();
				}

				if (ImGui::MenuItem("Open In Explorer"))
				{
					Platform::Get().OpenInExplorer(path.string());
				}

				ImGui::EndPopup();
			}

			if (fileToRename == path)
			{
				ImGui::SetNextItemWidth(cellSize - padding);
				ImGui::SetKeyboardFocusHere();

				if (ImGui::InputText("##Rename", fileRenameBuffer, IM_ARRAYSIZE(fileRenameBuffer), ImGuiInputTextFlags_EnterReturnsTrue))
				{
					pendingRenamePath = path;
					pendingRenameNewName = fileRenameBuffer;

					fileToRename = "";
				}

				if (ImGui::IsItemDeactivated() && !ImGui::IsKeyDown(ImGuiKey_Enter))
					fileToRename = "";
			}
			else
			{
				std::string displayFilename = filename;
				float availableWidth = cellSize - padding;

				if (ImGui::CalcTextSize(displayFilename.c_str()).x > availableWidth)
				{
					while (ImGui::CalcTextSize((displayFilename + "...").c_str()).x > availableWidth && displayFilename.length() > 1)
					{
						displayFilename.pop_back();
					}
					displayFilename += "...";
				}

				float textWidth = ImGui::CalcTextSize(filename.c_str()).x;
				float indent = (availableWidth - textWidth) * 0.5f;

				if (indent > 0)
					ImGui::SetCursorPosX(ImGui::GetCursorPosX() + indent);

				ImGui::Text("%s", displayFilename.c_str());

				if (ImGui::IsItemHovered())
				{
					ImGui::BeginTooltip();
					ImGui::Text("%s", filename.c_str());
					ImGui::EndTooltip();
				}

			}

			ImGui::NextColumn();
			ImGui::PopID();
		}

		if (!pendingRenamePath.empty())
		{
			std::filesystem::path newPath = pendingRenamePath.parent_path() / pendingRenameNewName;

			if (pendingRenamePath.has_extension() && newPath.extension() != pendingRenamePath.extension())
				newPath.replace_extension(pendingRenamePath.extension());

			std::error_code errorCode;
			if (std::filesystem::exists(pendingRenamePath))
			{
				std::filesystem::rename(pendingRenamePath, newPath, errorCode);
				if (!errorCode)
				{
					if (newPath.extension() == ".material")
					{
						std::string oldMaterialName = pendingRenamePath.stem().string();
						std::string newMaterialName = newPath.stem().string();

						ResourceManager::RenameMaterial(oldMaterialName, newMaterialName);

						auto materialToUpdate = ResourceManager::GetMaterialData(newMaterialName);
						if (materialToUpdate)
						{
							MaterialSerializer::Serialize(newPath.string(), materialToUpdate);
						}
					}

					// Make sure inspector doesn't lose reference to renamed file
					if (selectedFile == pendingRenamePath)
					{
						selectedFile = newPath;
						lastSelectedFile = newPath;
					}
				}
				else
				{
					SUNTA_ENGINE_LOG_ERROR("File Rename error: {0}", errorCode.message());
				}
			}

			pendingRenamePath = "";
		}
	}

	if (ImGui::BeginPopupContextWindow("BrowserEmptyMenu", ImGuiPopupFlags_NoOpenOverItems))
	{
		if (ImGui::MenuItem("Create New Folder"))
		{
			isCreatingFolder = true;
			strcpy(newFolderName, "New Folder");
		}

		if (ImGui::MenuItem("Create New Material"))
		{
			std::filesystem::path materialPath = currentDirectory / "NewMaterial.material";

			int index = 1;
			while (std::filesystem::exists(materialPath))
			{
				materialPath = currentDirectory / ("NewMaterial_" + std::to_string(index++) + ".material");
			}

			auto defaultShader = ResourceManager::GetShaderData(EngineAssets::Shaders::Lit);
			auto newMaterial = std::make_shared<Material>(defaultShader);
			newMaterial->SetName(materialPath.stem().string());

			MaterialSerializer::Serialize(materialPath.string(), newMaterial);
			ResourceManager::LoadMaterial(materialPath.stem().string(), newMaterial);
		}

		ImGui::EndPopup();
	}

	if (isCreatingFolder)
	{
		float iconSize = cellSize - padding;
		float availableWidth = cellSize - padding;
		float indent = (availableWidth - iconSize) * 0.5f;

		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + indent);
		auto icon = ResourceManager::GetEditorIcon(Sunta::EngineAssets::Icons::DefaultFolder);
		ImTextureID textureID = (ImTextureID)(uintptr_t)icon->GetID();
		ImGui::ImageButton("##NewFolderIcon", textureID, ImVec2(cellSize - padding, cellSize - padding));

		ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.0f, 0.4f, 0.8f, 0.6f));
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));

		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + indent);
		ImGui::SetNextItemWidth(iconSize);

		ImGui::SetKeyboardFocusHere();
		if (ImGui::InputText("##CreateFolderInput", newFolderName, IM_ARRAYSIZE(newFolderName), ImGuiInputTextFlags_EnterReturnsTrue))
		{
			std::filesystem::path newPath = currentDirectory / newFolderName;
			if (!std::filesystem::exists(newPath))
			{
				std::filesystem::create_directory(newPath);
				selectedFile = newPath;
			}

			isCreatingFolder = false;
		}

		if (ImGui::IsItemDeactivated() && !ImGui::IsKeyDown(ImGuiKey_Enter))
		{
			isCreatingFolder = false;
		}

		ImGui::PopStyleColor(2);
	}

	if (IsClickingEmptySpace())
	{
		selectedEntity = -1;
		selectedFile = "";
	}

	ImGui::Columns(1); // column reset
}

void EditorGUI::SetDarkTheme()
{
	auto& style = ImGui::GetStyle();
	auto* colors = style.Colors;

	// =========================================================
	// 1. Style Geometry & Padding
	// =========================================================
	style.WindowRounding = 0.0f; // Rounding of window corners
	style.FrameRounding = 2.0f; // Rounding of buttons and input frames
	style.ChildRounding = 0.0f; // Rounding of child windows
	style.GrabRounding = 2.0f; // Rounding of slider grabs

	style.WindowBorderSize = 0.0f; // Outer border size for active windows
	style.FrameBorderSize = 1.0f; // Border size around interactive frames
	style.PopupBorderSize = 1.0f; // Border size around popup menus
	style.TabBorderSize = 0.0f; // Border size for tabs

	style.WindowPadding = ImVec2(8.0f, 8.0f); // Outer padding inside windows
	style.FramePadding = ImVec2(4.0f, 3.0f); // Inner padding inside widgets
	style.ItemSpacing = ImVec2(8.0f, 4.0f); // Spacing between widgets
	style.ItemInnerSpacing = ImVec2(4.0f, 4.0f); // Spacing inside multi-part widgets

	// =========================================================
	// 2. Windows & Backgrounds
	// =========================================================
	colors[ImGuiCol_WindowBg] = ImVec4(0.08f, 0.07f, 0.06f, 1.00f); // Main window background
	colors[ImGuiCol_ChildBg] = ImVec4(0.05f, 0.04f, 0.04f, 1.00f); // Child window background
	colors[ImGuiCol_PopupBg] = ImVec4(0.10f, 0.08f, 0.07f, 1.00f); // Context menus and popups

	// =========================================================
	// 3. Title Bars & Menu Bar
	// =========================================================
	colors[ImGuiCol_TitleBg] = ImVec4(0.12f, 0.10f, 0.08f, 1.00f); // Inactive title bar
	colors[ImGuiCol_TitleBgActive] = ImVec4(0.18f, 0.12f, 0.10f, 1.00f); // Focused title bar
	colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.08f, 0.07f, 0.06f, 0.75f); // Collapsed title bar
	colors[ImGuiCol_MenuBarBg] = ImVec4(0.10f, 0.08f, 0.07f, 1.00f); // Main menu bar background

	// =========================================================
	// 4. Borders & Separators
	// =========================================================
	colors[ImGuiCol_Border] = ImVec4(0.30f, 0.05f, 0.05f, 0.80f); // Dark red border tint
	colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f); // Border drop shadow
	colors[ImGuiCol_Separator] = colors[ImGuiCol_Border];            // Default separator color
	colors[ImGuiCol_SeparatorHovered] = ImVec4(0.50f, 0.15f, 0.15f, 1.00f); // Separator on mouse hover
	colors[ImGuiCol_SeparatorActive] = ImVec4(0.50f, 0.20f, 0.15f, 1.00f); // Separator when clicked/dragged

	// =========================================================
	// 5. Input Fields & Text Selection
	// =========================================================
	colors[ImGuiCol_FrameBg] = ImVec4(0.12f, 0.10f, 0.09f, 1.00f); // Background for input boxes & checkmarks
	colors[ImGuiCol_FrameBgHovered] = ImVec4(0.20f, 0.15f, 0.13f, 1.00f); // Hovered input box background
	colors[ImGuiCol_FrameBgActive] = ImVec4(0.25f, 0.18f, 0.15f, 1.00f); // Active/editing input box background
	colors[ImGuiCol_TextSelectedBg] = ImVec4(0.50f, 0.20f, 0.20f, 0.43f); // Selected text highlight color

	// =========================================================
	// 6. Buttons & Collapsible Headers
	// =========================================================
	colors[ImGuiCol_Button] = ImVec4(0.18f, 0.14f, 0.12f, 1.00f); // Normal button color
	colors[ImGuiCol_ButtonHovered] = ImVec4(0.30f, 0.18f, 0.15f, 1.00f); // Hovered button color
	colors[ImGuiCol_ButtonActive] = ImVec4(0.50f, 0.15f, 0.15f, 1.00f); // Pressed button color

	colors[ImGuiCol_Header] = ImVec4(0.20f, 0.12f, 0.10f, 1.00f); // CollapsingHeader / TreeNode header
	colors[ImGuiCol_HeaderHovered] = ImVec4(0.35f, 0.15f, 0.12f, 1.00f); // Hovered header
	colors[ImGuiCol_HeaderActive] = ImVec4(0.50f, 0.20f, 0.15f, 1.00f); // Selected/Active header

	// =========================================================
	// 7. Checkmarks, Sliders & Resize Handles
	// =========================================================
	colors[ImGuiCol_CheckMark] = ImVec4(0.80f, 0.20f, 0.20f, 1.00f); // Checkbox tick color
	colors[ImGuiCol_SliderGrab] = ImVec4(0.60f, 0.20f, 0.20f, 1.00f); // Slider handle color
	colors[ImGuiCol_SliderGrabActive] = ImVec4(0.90f, 0.25f, 0.25f, 1.00f); // Slider handle while dragging

	colors[ImGuiCol_ResizeGrip] = ImVec4(0.18f, 0.14f, 0.12f, 1.00f); // Window resize corner handle
	colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.30f, 0.18f, 0.15f, 1.00f); // Hovered resize handle
	colors[ImGuiCol_ResizeGripActive] = ImVec4(0.50f, 0.15f, 0.15f, 1.00f); // Active resize handle

	// =========================================================
	// 8. Tabs & Tab Bars
	// =========================================================
	colors[ImGuiCol_Tab] = ImVec4(0.12f, 0.10f, 0.08f, 1.00f); // Inactive tab
	colors[ImGuiCol_TabHovered] = ImVec4(0.25f, 0.15f, 0.12f, 1.00f); // Hovered tab
	colors[ImGuiCol_TabActive] = ImVec4(0.12f, 0.10f, 0.08f, 1.00f); // Active/focused tab
	colors[ImGuiCol_TabUnfocused] = ImVec4(0.08f, 0.07f, 0.06f, 1.00f); // Inactive tab in unfocused window
	colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.12f, 0.10f, 0.08f, 1.00f); // Active tab in unfocused window

	colors[ImGuiCol_TabSelectedOverline] = ImVec4(0.50f, 0.15f, 0.15f, 1.00f); // Red accent overline on active tab
	colors[ImGuiCol_TabDimmedSelectedOverline] = ImVec4(0.20f, 0.12f, 0.10f, 1.00f); // Accent overline on unfocused active tab

	// =========================================================
	// 9. Scrollbars
	// =========================================================
	colors[ImGuiCol_ScrollbarBg] = ImVec4(0.05f, 0.04f, 0.04f, 1.00f); // Scrollbar track background
	colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.18f, 0.14f, 0.12f, 1.00f); // Scrollbar grab handle
	colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.30f, 0.18f, 0.15f, 1.00f); // Hovered scrollbar grab handle
	colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.50f, 0.15f, 0.15f, 1.00f); // Active scrollbar grab handle

	// =========================================================
	// 10. Docking & Drag and Drop
	// =========================================================
	colors[ImGuiCol_DockingPreview] = ImVec4(0.50f, 0.20f, 0.15f, 0.40f); // Preview area when docking windows
	colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.08f, 0.07f, 0.06f, 1.00f); // Background for empty docking nodes
	colors[ImGuiCol_DragDropTarget] = ImVec4(0.80f, 0.20f, 0.20f, 1.00f); // Target highlight when dragging elements

	// =========================================================
	// 11. Navigation & Focus Highlights
	// =========================================================
	colors[ImGuiCol_NavHighlight] = ImVec4(0.50f, 0.15f, 0.15f, 1.00f); // Keyboard/gamepad navigation highlight
	colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.20f); // Highlight during window switching (Ctrl+Tab)
	colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.05f, 0.04f, 0.04f, 0.20f); // Background dimming during window switching
}

void EditorGUI::DrawWindowBackground(const std::string& textureKey, glm::vec4 tintColor)
{
	const auto background = ResourceManager::GetEditorIcon(textureKey);
	if (!background)
		return;

	ImTextureID textureID = (ImTextureID)(uintptr_t)background->GetID();
	ImDrawList* drawList = ImGui::GetWindowDrawList();
	ImVec2 position = ImGui::GetWindowPos();
	ImVec2 size = ImGui::GetWindowSize();

	float textureWidth = static_cast<float>(background->GetWidth());
	float textureHeight = static_cast<float>(background->GetHeight());

	// UV = teture coordinates (0.0 to 1.0), scaling by (windowSize /  textureSize) repeats texture (tiling)
	ImVec2 uv0 = ImVec2(0, 0); // 0.0 in textures is top left corner)
	ImVec2 uv1 = ImVec2(size.x / textureWidth, size.y / textureHeight); // 1.0 in textures is bottom right corner

	ImVec4 imguiColor = ImVec4(tintColor.r, tintColor.g, tintColor.b, tintColor.a);

	drawList->AddImage(textureID, position, ImVec2(position.x + size.x, position.y + size.y),
		ImVec2(0, 0), ImVec2(size.x / textureWidth, size.y / textureHeight),
		ImGui::GetColorU32(imguiColor));

}

std::string EditorGUI::GetIconKeyForPath(const std::filesystem::path& path, bool isDirectory)
{
	using namespace Sunta::EngineAssets;

	if (isDirectory)
	{
		static const std::unordered_map<std::string, std::string> folderIcons = 
		{
			{ "scripts",		"cpp_folder" },			{ "src",		  "cpp_folder" },		{ "cpp",	"cpp_folder" },
			{ "models",			"3d_model_folder" },	{ "meshes",	 "3d_model_folder" },
			{ "shaders",		"shader_folder" },
			{ "textures",		"image_folder" },		{ "sprites",	"image_folder" },		{ "images", "image_folder" },
			{ "audio",			"audio_folder" },		{ "sounds",		"audio_folder" },		{ "sfx",	"audio_folder" },
			{ "fonts",			"fonts_folder" }
		};

		std::string name = path.filename().string();
		std::transform(name.begin(), name.end(), name.begin(), ::tolower);

		auto it = folderIcons.find(name);
		return (it != folderIcons.end()) ? it->second : Icons::DefaultFolder;
	}
	else
	{
		static const std::unordered_map<std::string, std::string> fileIcons =
		{
			{".shader",		"shader_file"},
			{".png",		"image_file"},		{".jpg", "image_file"},		{".jpeg", "image_file"},
			{".obj",		"3d_model_file"},	{".fbx", "3d_model_file"},
			{".cpp",		"cpp_file"},		{".h", "cpp_file"},
			{".wav",		"audio_file"},		{".ogg", "audio_file"},
			{".ttf",		"font_file"}
		};

		std::string extension = path.extension().string();
		std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);

		auto it = fileIcons.find(extension);
		return (it != fileIcons.end()) ? it->second : Icons::DefaultFile;

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
	case PropertyDataType::AssetPath:
		{
			std::string& currentPath = *(std::string*)propertyData;
			std::vector<std::string> options;

			if (property.assetType == AssetType::Mesh)
				options = ResourceManager::GetMeshesNames();
			else if (property.assetType == AssetType::Material)
				options = ResourceManager::GetMaterialsNames();

			if (ImGui::BeginCombo(property.label.c_str(), currentPath.c_str()))
			{
				for (const auto& option : options)
				{
					bool isSelected = (currentPath == option);
					if (ImGui::Selectable(option.c_str(), isSelected))
					{
						currentPath = option;
						changed = true;
					}
				}

				ImGui::EndCombo();
			}

			if (ImGui::BeginDragDropTarget())
			{
				if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("FILE_PATH"))
				{
					std::string droppedPath = (const char*)payload->Data;

					currentPath = droppedPath;
					changed = true;
				}

				ImGui::EndDragDropTarget();
			}

		}
		break;
	}
	
	return changed;
}

void EditorGUI::HandleSelectionInteraction()
{
	if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
	{
		if (!ImGui::GetIO().WantCaptureMouse)
		{
			ClearSelection();
		}
	}
}

void EditorGUI::ClearSelection()
{
	selectedEntity = -1;
	selectedFile = "";
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

bool EditorGUI::IsClickingEmptySpace()
{
	return ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !ImGui::IsAnyItemHovered();
}


}