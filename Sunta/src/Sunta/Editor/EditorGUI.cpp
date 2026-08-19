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
#include "Events/EventTypes.h"
#include "Serialization/SceneSerializer.h"
#include "Utilities/FileSystemUtilities.h"

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

void EditorGUI::DrawMainMenuBar(Scene& scene)
{
	if (ImGui::BeginMainMenuBar())
	{
		if (ImGui::BeginMenu("File"))
		{
			if (ImGui::MenuItem("New Scene"))
			{
				scene.Clear();
				scene.SetName("Untitled_Scene");
			}

			if (ImGui::MenuItem("Open Scene", "Ctrl+O"))
			{
				std::string path = Platform::Get().OpenFileDialog("Scene Files", { "scene" }, "res/Game/Scenes");
				if (!path.empty())
					SceneSerializer::Deserialize(path, scene);
			}

			ImGui::Separator();

			if (ImGui::MenuItem("Save Scene", "Ctrl+S"))
			{
				std::string path = "res/Game/Scenes/" + scene.GetName() + ".scene";
				SceneSerializer::Serialize(path, scene);
			}

			if (ImGui::MenuItem("Save Scene As", "Ctrl+Shift+S"))
			{
				std::string path = Platform::Get().SaveFileDialog("Scene Files", { "scene" }, "res/Game/Scenes");
				if (!path.empty())
				{
					std::filesystem::path filepath = std::filesystem::path(path).lexically_normal();
					scene.SetName(filepath.stem().string());
					SceneSerializer::Serialize(filepath.string(), scene);
				}
			}

			ImGui::EndMenu();
		}

		ImGui::EndMainMenuBar();
	}

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
				EntityFactory::CreateCube(scene, glm::vec3(0.0f, 2.0f, 0.0f), "Cube");
			});

		DrawToolbarButton(Icons::Pyramid, "##CreatePyramid", "Create Pyramid Entity", [&]()
			{
				EntityFactory::CreatePyramid(scene, glm::vec3(0.0f, 2.0f, 0.0f), "Pyramid");
			});

		DrawToolbarButton(Icons::Cone, "##CreateCone", "Create Cone Entity", [&]()
			{
				EntityFactory::CreateCone(scene, glm::vec3(0.0f, 2.0f, 0.0f), "Cone");
			});

		DrawToolbarButton(Icons::Sphere, "##CreateSphere", "Create Sphere Entity", [&]()
			{
				EntityFactory::CreateSphere(scene, glm::vec3(0.0f, 2.0f, 0.0f), "Sphere");
			});

		DrawToolbarButton(Icons::Capsule, "##CreateCapsule", "Create Capsule Entity", [&]()
			{
				EntityFactory::CreateCapsule(scene, glm::vec3(0.0f, 2.0f, 0.0f), "Capsule");
			});

		DrawToolbarButton(Icons::DirectionalLight, "##CreateDirectionalLight", "Create Directional Light", [&]()
			{
				EntityFactory::CreateDirectionalLight(scene, glm::vec3(0.0f, 2.0f, 0.0f), "Directional Light");
			});
		
		DrawToolbarButton(Icons::PointLight, "##CreatePointLight", "Create Point Light", [&]()
			{
				EntityFactory::CreatePointLight(scene, glm::vec3(0.0f, 2.0f, 0.0f), "Point Light");
			});

		DrawToolbarButton(Icons::Spotlight, "##CreateSpotLight", "Create Spot Light", [&]()
			{
				EntityFactory::CreateSpotLight(scene, glm::vec3(0.0f, 2.0f, 0.0f), "Spot Light");
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

	}

	ImGui::End();

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
				snprintf(entityNameBuffer, sizeof(entityNameBuffer), "%s", label.c_str());
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
				snprintf(entityNameBuffer, sizeof(entityNameBuffer), "%s", label.c_str());
			}

			if (ImGui::MenuItem("Delete"))
			{
				// TODO: add implementation for Destroy Entity in entity Manager
				//entityManager.DestroyEntity(entityID);
				SUNTA_ENGINE_LOG_INFO("Deleted entity: '{0}'", selectedEntity);
				selectedEntity = -1;

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

	if (selectedEntity != -1 && entityToRename == -1)
	{
		if (ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows) && ImGui::IsKeyPressed(ImGuiKey_Delete))
		{
			SUNTA_ENGINE_LOG_INFO("Deleted entity: '{0}'", selectedEntity);
			selectedEntity = -1;
		}
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

			changed |= ImGui::ColorEdit3("Ambient", &data.ambientColor.r);
			changed |= ImGui::ColorEdit3("Diffuse", &data.diffuseColor.r);
			changed |= ImGui::ColorEdit3("Specular", &data.specularColor.r);
			changed |= ImGui::DragFloat("Shininess", &data.shininess, 0.5f, 1.0f, 256.0f);

			changed |= ImGui::Checkbox("Use Alpha Cutout", &data.useAlphaCutout);

			ImGui::Spacing();
			ImGui::Separator();
			ImGui::Text("Textures");
			ImGui::Spacing();

			const auto& diffuseMaps = currentMaterial->GetDiffuseMaps();
			std::shared_ptr<Texture> currentDiffuse = diffuseMaps.empty() ? nullptr : diffuseMaps[0];

			if (DrawTextureSlot("Diffuse Map", currentDiffuse))
			{
				currentMaterial->SetDiffuseMap(currentDiffuse, 0);
				changed = true;
			}

			ImGui::Spacing();

			const auto& specularMaps = currentMaterial->GetSpecularMaps();
			std::shared_ptr<Texture> currentSpecular = specularMaps.empty() ? nullptr : specularMaps[0];

			if (DrawTextureSlot("Specular Map", currentSpecular))
			{
				currentMaterial->SetSpecularMap(currentSpecular, 0);
				changed = true;
			}

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

	ImGui::Separator();

	//										-1.0f stretches X along whole width
	if (ImGui::Button("Add Component", ImVec2(-1.0f, 30.0f)))
	{
		ImGui::OpenPopup("AddComponentPopup");
	}

	if(ImGui::BeginPopup("AddComponentPopup"))
	{
		struct ComponentAction
		{
			std::string name;
			std::function<bool()> hasComponent;
			std::function<void()> addComponent;
		};

		ComponentAction availableComponents[] =
		{
			{
				"Mesh",
				[&]() { return entityManager.GetComponent<MeshComponent>(selectedEntity) != nullptr; },
				[&]() { return entityManager.AddComponent<MeshComponent>(selectedEntity); }
			},

			{
				"Directional Light",
				[&]() { return entityManager.GetComponent<DirectionalLightComponent>(selectedEntity) != nullptr; },
				[&]() { return entityManager.AddComponent<DirectionalLightComponent>(selectedEntity); }
			},

			{
				"Point Light",
				[&]() { return entityManager.GetComponent<PointLightComponent>(selectedEntity) != nullptr; },
				[&]() { return entityManager.AddComponent<PointLightComponent>(selectedEntity); }
			},

			{
				"Spot Light",
				[&]() { return entityManager.GetComponent<SpotLightComponent>(selectedEntity) != nullptr; },
				[&]() { return entityManager.AddComponent<SpotLightComponent>(selectedEntity); }
			},

			{
				"Script",
				[&]() { return entityManager.GetComponent<ScriptComponent>(selectedEntity) != nullptr; },
				[&]() { return entityManager.AddComponent<ScriptComponent>(selectedEntity); }
			}
		};

		for (const auto& action : availableComponents)
		{
			if (!action.hasComponent())
			{
				if (ImGui::MenuItem(action.name.c_str()))
				{
					action.addComponent();
					ImGui::CloseCurrentPopup();
				}
			}
		}

		ImGui::EndPopup();
	}


	// here we stop using that uniqueID for ImGUI and we go back to defualt mode
	ImGui::PopID();

}

void EditorGUI::DrawScriptComponentInspector(ScriptComponent& scriptComponent)
{
	if (ImGui::CollapsingHeader("Script Component"))
	{
		if (ImGui::Button("Add Script"))
			scriptComponent.scripts.push_back(ScriptContainer{});

		ImGui::Separator();

		for (unsigned int i = 0; i < scriptComponent.scripts.size(); i++)
		{
			ImGui::PushID(static_cast<int>(i));

			char buffer[256];
			memset(buffer, 0, sizeof(buffer));
			strcpy_s(buffer, scriptComponent.scripts[i].scriptPath.c_str());

			if (ImGui::InputText("Script Path", buffer, sizeof(buffer)))
				scriptComponent.scripts[i].scriptPath = std::string(buffer);

		}
	}
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
			//selectedFile = "";
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
			ImGui::Columns(1);
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
					else
					{
						Platform::Get().OpenFileExternally(path.string());
					}
				}
			}

			if (isSelected && ImGui::IsKeyPressed(ImGuiKey_F2))
			{
				fileToRename = path;
				snprintf(fileRenameBuffer, sizeof(fileRenameBuffer), "%s", filename.c_str());
			}

			if (ImGui::BeginPopupContextItem("FileMenu"))
			{
				selectedFile = path;

				if (ImGui::MenuItem("Rename"))
				{
					fileToRename = path;
					snprintf(fileRenameBuffer, sizeof(fileRenameBuffer), "%s", filename.c_str());
				}

				if (ImGui::MenuItem("Delete"))
				{
					if (selectedFile == path)
						selectedFile = "";

					DeletePathAndUnloadResources(path);

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
		auto StartCreation = [](const std::string& baseName, const std::string& extension, const std::string& icon, auto onCreateFunc)
		{
			std::string uniqueName = baseName;
			int index = 1;

			auto BuildTestPath = [&](const std::string& name)
			{
				std::filesystem::path path = currentDirectory / name;
				if (!extension.empty() && path.extension() != extension)
					path.replace_extension(extension);

				return path;
			};

			while (std::filesystem::exists(BuildTestPath(uniqueName)))
			{
				uniqueName = baseName + "_" + std::to_string(index++);
			}

			pendingCreation.active = true;
			snprintf(pendingCreation.nameBuffer, sizeof(pendingCreation.nameBuffer), "%s", uniqueName.c_str());
			pendingCreation.extension = extension;
			pendingCreation.iconKey = icon;
			pendingCreation.onCreate = onCreateFunc;
		};

		if (ImGui::MenuItem("Create New Folder"))
		{
			StartCreation("New Folder", "", Sunta::EngineAssets::Icons::DefaultFolder, [](const std::filesystem::path& path)
				{
					if (!std::filesystem::exists(path))
					{
						std::filesystem::create_directory(path);
						selectedFile = path;
					}
				});
		}

		if (ImGui::MenuItem("Create New Material"))
		{
			StartCreation("New Material", ".material", Sunta::EngineAssets::Icons::DefaultFile, [](const std::filesystem::path& path)
				{
					if (!std::filesystem::exists(path))
					{
						auto defaultShader = ResourceManager::GetShaderData(EngineAssets::Shaders::Lit);
						auto newMaterial = std::make_shared<Material>(defaultShader);
						newMaterial->SetName(path.stem().string());

						MaterialSerializer::Serialize(path.string(), newMaterial);
						ResourceManager::LoadMaterial(path.stem().string(), newMaterial);
						selectedFile = path;
					}
				});

		}

		ImGui::EndPopup();
	}

	if (pendingCreation.active)
	{
		float iconSize = cellSize - padding;
		float availableWidth = cellSize - padding;
		float indent = (availableWidth - iconSize) * 0.5f;

		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + indent);
		auto icon = ResourceManager::GetEditorIcon(pendingCreation.iconKey);
		if (icon)
		{
			ImTextureID textureID = (ImTextureID)(uintptr_t)icon->GetID();
			ImGui::ImageButton("##NewItemIcon", textureID, ImVec2(iconSize, iconSize));
		}

		ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.0f, 0.4f, 0.8f, 0.6f));
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));

		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + indent);
		ImGui::SetNextItemWidth(iconSize);

		ImGui::SetKeyboardFocusHere();
		if (ImGui::InputText("##CreateItemInput", pendingCreation.nameBuffer, IM_ARRAYSIZE(pendingCreation.nameBuffer), ImGuiInputTextFlags_EnterReturnsTrue))
		{
			std::string finalName = pendingCreation.nameBuffer;

			if (!finalName.empty())
			{
				std::filesystem::path fullPath = currentDirectory / finalName;

				if (!pendingCreation.extension.empty() && fullPath.extension() != pendingCreation.extension)
					fullPath.replace_extension(pendingCreation.extension);

				if (pendingCreation.onCreate)
					pendingCreation.onCreate(fullPath);
			}

			pendingCreation.active = false;

		}

		if (ImGui::IsItemDeactivated() && !ImGui::IsKeyDown(ImGuiKey_Enter))
		{
			pendingCreation.active = false;
		}

		ImGui::PopStyleColor(2);
	}

	if (IsClickingEmptySpace())
	{
		selectedEntity = -1;
		selectedFile = "";
	}

	if (!selectedFile.empty() && fileToRename.empty() && !pendingCreation.active)
	{
		if (ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows) && ImGui::IsKeyPressed(ImGuiKey_Delete))
		{
			DeletePathAndUnloadResources(selectedFile);
			selectedFile = "";
		}
	}

	ImGui::Columns(1); // column reset
}

void EditorGUI::DrawSceneDropTarget(Scene& scene)
{
	if (!ImGui::IsDragDropActive())
		return;

	ImGuiViewport* viewport = ImGui::GetMainViewport();

	ImGui::SetNextWindowPos(viewport->Pos);
	ImGui::SetNextWindowSize(viewport->Size);
	ImGui::SetNextWindowBgAlpha(0.0f);

	ImGuiWindowFlags flags = 
		  ImGuiWindowFlags_NoTitleBar
		| ImGuiWindowFlags_NoResize
		| ImGuiWindowFlags_NoMove
		| ImGuiWindowFlags_NoScrollbar
		| ImGuiWindowFlags_NoSavedSettings
		| ImGuiWindowFlags_NoBringToFrontOnFocus
		| ImGuiWindowFlags_NoFocusOnAppearing
		| ImGuiWindowFlags_NoNav;

	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

	if (ImGui::Begin("##SceneDropTarget", nullptr, flags))
	{
		// Invisble element set to whole window that captures mouse
		ImGui::Dummy(viewport->Size);

		if (ImGui::BeginDragDropTarget())
		{
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("FILE_PATH"))
			{
				const char* pathString = static_cast<const char*>(payload->Data);
				std::filesystem::path droppedPath(pathString);

				std::string extension = droppedPath.extension().string();
				std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);

				// ===================================================================================
				// TODO: ADD RAYCASTING BASED ON MOUSE POSITON AND THEN CREATE ENTITY IN THAT POSITION
				// ===================================================================================
				ImVec2 mousePosition = ImGui::GetMousePos();
				SUNTA_ENGINE_LOG_INFO("Dropped file '{0}' onto Scene background at screen position: ({1}, {2})",
					droppedPath.string(), mousePosition.x, mousePosition.y);
				// ===================================================================================
				// ===================================================================================

				glm::vec3 spawnPosition = glm::vec3(0.0f, 0.0f, 0.0f);
				std::string entityName = droppedPath.stem().string();

				// ===================================================================================
				// TODO: ADD DIFFERENT EXTENSIONS HANDLING 
				// ===================================================================================

				if (IsModelExtension(extension))
				{
					// TODO: ADD CREATING 3D MODEL
					EntityFactory::CreateCube(scene, spawnPosition, entityName);
					SUNTA_ENGINE_LOG_INFO("Dropped 3D Model: '{0}' onto scene", droppedPath.string());
				}
				else if (IsMaterialExtension(extension))
				{
					// TODO: ADD HANDLING DROPPED MATERIAL
					auto material = ResourceManager::GetMaterialData(entityName);
					if(material)
						EntityFactory::CreateCube(scene, spawnPosition, entityName, material);

					SUNTA_ENGINE_LOG_INFO("Dropped material: '{0}'", entityName);
				}
				else if (IsTextureExtension(extension))
				{
					SUNTA_ENGINE_LOG_INFO("Dropped Texture: '{0}' onto scene", droppedPath.string());
				}
				else if (IsAudioExtension(extension))
				{
					SUNTA_ENGINE_LOG_INFO("Dropped Audio File: '{0}' onto scene. Creating AudioSource Entity...", droppedPath.string());
					unsigned int audioEntity = EntityFactory::CreateEmpty(scene, spawnPosition, entityName + "_Audio");
					// TODO: Add AudioSource Component
				}
				else
				{
					SUNTA_ENGINE_LOG_WARNING("Unsuported file extension '{0}' dropped onto scene.", extension);
						
				}
				
			}

			ImGui::EndDragDropTarget();
		}
	}

	ImGui::End();
	ImGui::PopStyleVar(2);

}

void EditorGUI::OnFileDropped(const FileDroppedEvent& event)
{
	std::filesystem::path targetDirectory = currentDirectory.empty() ? "res/Game" : currentDirectory;

	if (!std::filesystem::exists(targetDirectory))
	{
		std::filesystem::create_directories(targetDirectory);
	}

	for (const auto& pathString : event.paths)
	{
		std::filesystem::path srcPath(pathString);
		std::filesystem::path destinationPath = targetDirectory / srcPath.filename();

		std::error_code errorCode;
		std::filesystem::copy_file(srcPath, destinationPath, std::filesystem::copy_options::overwrite_existing, errorCode);

		if (errorCode)
		{
			SUNTA_ENGINE_LOG_ERROR("EditorGUI::OnFileDropped: Error Importing file '{0}': '{1}'", srcPath.filename().string(), errorCode.message());
		}
		else
		{
			SUNTA_ENGINE_LOG_INFO("EditorGUI::OnFileDropped: Successfully imported file to: '{0}'", destinationPath.string());
		}
	}
}

bool EditorGUI::DrawTextureSlot(const char* label, std::shared_ptr<Texture>& texture)
{
	bool changed = false;

	ImGui::PushID(label);
	ImGui::Text("%s", label);

	float slotSize = 60.0f;

	ImTextureID textureID = 0;
	if (texture)
	{
		textureID = (ImTextureID)(uintptr_t)texture->GetID();
	}
	else
	{
		auto defaultIcon = ResourceManager::GetEditorIcon(Sunta::EngineAssets::Icons::ImageFile);
		if (defaultIcon)
			textureID = (ImTextureID)(uintptr_t)defaultIcon->GetID();
	}

	if (textureID)
		ImGui::Image(textureID, ImVec2(slotSize, slotSize), ImVec2(0, 1), ImVec2(1, 0));
	else
		ImGui::Button("No Texture", ImVec2(slotSize, slotSize));

	if (ImGui::BeginDragDropTarget())
	{
		if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("FILE_PATH"))
		{
			std::string droppedPath = (const char*)payload->Data;
			std::filesystem::path path(droppedPath);
			std::string extension = path.extension().string();
			std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);

			if (IsTextureExtension(extension))
			{
				texture = ResourceManager::LoadOrGetTexture(droppedPath);
				changed = true;
			}
		}

		ImGui::EndDragDropTarget();
	}

	ImGui::SameLine();
	ImGui::BeginGroup();

	if (texture)
	{
		std::string filename = std::filesystem::path(texture->GetFilePath()).filename().string();
		ImGui::TextUnformatted(filename.c_str());

		if (ImGui::Button("Remove"))
		{
			texture = nullptr;
			changed = true;
		}
	}
	else
	{
		ImGui::TextDisabled("None (Texture)");
		ImGui::TextDisabled("Drag & Drop Image here");
	}

	if (ImGui::Button("Browse..."))
	{
		std::string selectedPath = Platform::Get().OpenFileDialog("Texture Files", {"png", "jpg", "jpeg", "tga", "bmp", "psd", "hdr"});

		if (!selectedPath.empty())
		{
			texture = ResourceManager::LoadOrGetTexture(selectedPath);
			changed = true;
		}
	}

	ImGui::EndGroup();
	ImGui::PopID();

	return changed;
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
		uv0, uv1,
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
			{ "materials",		"image_folder" },
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
			{".cpp",		"cpp_file"},		{".h", "cpp_file"},
			{".obj",		"3d_model_file"},	{".fbx", "3d_model_file"},
			{".shader",		"shader_file"},
			{".png",		"image_file"},		{".jpg", "image_file"},	    {".jpeg", "image_file"}, {".tga","image_file"}, {".bmp", "image_file"}, {".psd", "image_file"}, {".hdr", "image_file"},
			{".material",	"image_file"},
			{".wav",		"audio_file"},		{".ogg", "audio_file"},
			{".ttf",		"font_file"}
		};

		std::string extension = path.extension().string();
		std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);

		auto it = fileIcons.find(extension);
		return (it != fileIcons.end()) ? it->second : Icons::DefaultFile;

	}
}

void EditorGUI::DeletePathAndUnloadResources(const std::filesystem::path& path)
{
	if (!std::filesystem::exists(path))
		return;

	if (std::filesystem::is_directory(path))
	{
		std::error_code error;
		for (const auto& entry : std::filesystem::recursive_directory_iterator(path, error))
		{
			if (entry.is_regular_file())
			{
				ResourceManager::UnloadResourceByPath(entry.path());
			}
		}
	}
	else
	{
		ResourceManager::UnloadResourceByPath(path);
	}

	std::error_code errorCode;
	std::filesystem::remove_all(path, errorCode);

	if (errorCode)
	{
		SUNTA_ENGINE_LOG_ERROR("EditorGUI::DeletePathAndUnloadResources: Error deleting '{0}': '{1}'", 
			path.string(), errorCode.message());
	}
	else
	{
		SUNTA_ENGINE_LOG_INFO("EditorGUI::DeletePathAndUnloadResources: Successfully deletd: '{0}'", path.string());
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
	auto icon = ResourceManager::GetEditorIcon(componentType->iconKey);

	ImVec2 cursorPosition = ImGui::GetCursorScreenPos();
	float frameHeight = ImGui::GetFrameHeight();
	float fontSize = ImGui::GetFontSize();
	const ImGuiStyle& style = ImGui::GetStyle();

	// ===============================================
	//				INDENT CONFIG			        ||
	// ===============================================
	float iconSize           = 24.0f;             //||
	float arrowToIconSpacing = 2.0f;			  //||
	float iconToTextSpacing  = 6.0f;			  //||
	// ===============================================

	std::string headerID = "###Header_" + componentType->name;
	bool isOpen = ImGui::CollapsingHeader(headerID.c_str(), ImGuiTreeNodeFlags_DefaultOpen);

	ImDrawList* drawList = ImGui::GetWindowDrawList();

	float arrowOffset = style.FramePadding.x + fontSize + arrowToIconSpacing;
	float currentX = cursorPosition.x + arrowOffset;
	
	if (icon)
	{
		float iconOffsetY = (frameHeight - iconSize) * 0.5f;
		ImTextureID textureID = (ImTextureID)(uintptr_t)icon->GetID();

		drawList->AddImage(textureID,
			ImVec2(currentX           , cursorPosition.y + iconOffsetY),
			ImVec2(currentX + iconSize, cursorPosition.y + iconOffsetY + iconSize));

		currentX += iconSize + iconToTextSpacing;
	}

	float textOffsetY = (frameHeight - fontSize) * 0.5f;
	drawList->AddText(
		ImVec2(currentX, cursorPosition.y + textOffsetY), 
		ImGui::GetColorU32(ImGuiCol_Text), 
		componentType->name.c_str());

	if (!isOpen)
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