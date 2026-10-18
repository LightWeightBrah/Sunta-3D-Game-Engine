#include "Core/SuntaPreCompiled.h"
#include "ProjectLauncherLayer.h"

#include <imgui/imgui.h>

#include "EditorGUI.h"
#include "EditorGUIContext.h"
#include "EditorSettings.h"
#include "ImGuiTextInput.h"
#include "ProcessLauncher.h"
#include "ProjectManager.h"
#include "Core/Application.h"
#include "Core/InputManager.h"
#include "Core/Log.h"
#include "Events/EventBus.h"
#include "Events/EventTypes.h"

namespace Sunta
{

namespace
{

namespace fs = std::filesystem;

constexpr const char* WindowName         = "Project Launcher";
constexpr const char* RecentListName     = "RecentProjects";
constexpr const char* NewProjectPopup    = "New Project";

constexpr float TitleFontScale   = 1.8f;
constexpr float ButtonsAreaWidth = 190.0f;   // "Open" + "Remove" buttons of a row
constexpr float FooterLines      = 4.0f;     // space under the list for the buttons and the status

constexpr ImGuiWindowFlags FullWindowFlags = ImGuiWindowFlags_NoDecoration
										   | ImGuiWindowFlags_NoMove
										   | ImGuiWindowFlags_NoSavedSettings
										   | ImGuiWindowFlags_NoBringToFrontOnFocus;

const ImVec4 ErrorColor = ImVec4(1.0f, 0.45f, 0.4f, 1.0f);

fs::path sampleProjectDirectory;

// EditorGUIContext::Init adds the editor font as the last one, the first is the tiny default font of ImGui
ImFont* GetEditorFont()
{
	ImFontAtlas* fonts = ImGui::GetIO().Fonts;
	return (fonts->Fonts.Size > 1) ? fonts->Fonts.back() : nullptr;
}

}

ProjectLauncherLayer::ProjectLauncherLayer(Application& application)
	: application(application)
{

}

void ProjectLauncherLayer::SetSampleProjectDirectory(const fs::path& directory)
{
	sampleProjectDirectory = directory;
}

void ProjectLauncherLayer::OnAttach()
{
	EditorGUIContext::Init(&application.GetWindow());
	EditorGUI::SetDarkTheme();

	// There is no scene to fly around in: shows the cursor and locks the camera
	EventBus::Publish<EngineModeChangedEvent>({ EngineMode::Editor });

	newProjectLocation = sampleProjectDirectory.empty() ? fs::current_path() : sampleProjectDirectory.parent_path();
	RefreshRecentProjects();
}

void ProjectLauncherLayer::OnDetach()
{
	EditorGUIContext::Shutdown(&application.GetWindow());
}

void ProjectLauncherLayer::OnProcessInput()
{
	if (InputManager::IsKeyDown(GLFW_KEY_ESCAPE))
		application.Close();
}

void ProjectLauncherLayer::OnRender()
{
	Window* window = &application.GetWindow();

	EditorGUIContext::NewFrame(window);

	ImFont* font = GetEditorFont();
	if (font)
		ImGui::PushFont(font);

	DrawLauncherWindow();

	if (font)
		ImGui::PopFont();

	EditorGUIContext::EndFrame(window);
}

ProjectLauncherLayer::ProjectEntry ProjectLauncherLayer::CreateEntry(const fs::path& directory) const
{
	ProjectEntry entry;
	entry.directory = directory;
	entry.pathText  = directory.string();
	entry.exists    = ProjectManager::IsProjectDirectory(directory);
	entry.name      = entry.exists ? ProjectManager::ReadProjectName(directory) : directory.filename().string();

	return entry;
}

// Reads game.config of every project, so it is done when the list changes, not every frame
void ProjectLauncherLayer::RefreshRecentProjects()
{
	recentProjects.clear();

	for (const fs::path& directory : EditorSettings::GetRecentProjects())
		recentProjects.push_back(CreateEntry(directory));
}

void ProjectLauncherLayer::DrawLauncherWindow()
{
	const ImGuiViewport* viewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(viewport->WorkPos);
	ImGui::SetNextWindowSize(viewport->WorkSize);

	if (ImGui::Begin(WindowName, nullptr, FullWindowFlags))
	{
		DrawTitle();
		DrawRecentProjects();
		DrawActions();
		DrawNewProjectPopup();
	}
	ImGui::End();
}

void ProjectLauncherLayer::DrawTitle()
{
	ImGui::SetWindowFontScale(TitleFontScale);
	ImGui::TextUnformatted("Sunta Engine");
	ImGui::SetWindowFontScale(1.0f);

	ImGui::TextDisabled("Choose a project to open in the editor");
	ImGui::Spacing();
}

void ProjectLauncherLayer::DrawRecentProjects()
{
	ImGui::TextUnformatted("Recent projects");

	const float footerHeight = ImGui::GetFrameHeightWithSpacing() * FooterLines;
	ImGui::BeginChild(RecentListName, ImVec2(0.0f, -footerHeight), true);

	if (recentProjects.empty())
		ImGui::TextDisabled("Nothing here yet. Create a new project or open an existing one.");

	// Removing inside the loop would change the list while it is drawn
	fs::path projectToRemove;

	for (size_t index = 0; index < recentProjects.size(); ++index)
	{
		ImGui::PushID(static_cast<int>(index));
		DrawRecentProjectRow(recentProjects[index], projectToRemove);
		ImGui::PopID();
	}

	ImGui::EndChild();

	if (!projectToRemove.empty())
	{
		EditorSettings::RemoveRecentProject(projectToRemove);
		RefreshRecentProjects();
	}
}

void ProjectLauncherLayer::DrawRecentProjectRow(const ProjectEntry& project, fs::path& outProjectToRemove)
{
	ImGui::BeginGroup();
	ImGui::TextUnformatted(project.name.c_str());
	ImGui::TextDisabled("%s%s", project.pathText.c_str(), project.exists ? "" : "   (folder not found)");
	ImGui::EndGroup();

	ImGui::SameLine(ImGui::GetContentRegionMax().x - ButtonsAreaWidth);

	ImGui::BeginDisabled(!project.exists);
	if (ImGui::Button("Open"))
		OpenProject(project.directory);
	ImGui::EndDisabled();

	ImGui::SameLine();
	if (ImGui::Button("Remove from list"))
		outProjectToRemove = project.directory;

	ImGui::Separator();
}

void ProjectLauncherLayer::DrawActions()
{
	if (ImGui::Button("New Project..."))
		shouldOpenNewProjectPopup = true;

	ImGui::SameLine();
	if (ImGui::Button("Open Project..."))
		OpenProjectFromFolderDialog();

	const bool hasSampleProject = !sampleProjectDirectory.empty() && ProjectManager::IsProjectDirectory(sampleProjectDirectory);
	if (hasSampleProject)
	{
		ImGui::SameLine();
		if (ImGui::Button("Open Sample Project"))
			OpenProject(sampleProjectDirectory);
	}

	bool shouldOpenLastProject = EditorSettings::ShouldOpenLastProjectOnStartup();
	if (ImGui::Checkbox("Open the last project at startup (this window can be shown again from the editor: Project > Switch Project)", &shouldOpenLastProject))
		EditorSettings::SetOpenLastProjectOnStartup(shouldOpenLastProject);

	if (!statusMessage.empty())
		ImGui::TextColored(ErrorColor, "%s", statusMessage.c_str());
}

void ProjectLauncherLayer::DrawNewProjectPopup()
{
	if (shouldOpenNewProjectPopup)
	{
		newProjectError.clear();
		ImGui::OpenPopup(NewProjectPopup);
		shouldOpenNewProjectPopup = false;
	}

	const ImGuiViewport* viewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(viewport->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

	if (!ImGui::BeginPopupModal(NewProjectPopup, nullptr, ImGuiWindowFlags_AlwaysAutoResize))
		return;

	InputString("Name", newProjectName);

	ImGui::Text("Location: %s", newProjectLocation.string().c_str());
	ImGui::SameLine();
	if (ImGui::Button("Browse..."))
	{
		const fs::path chosenFolder = ProjectManager::BrowseForFolder(newProjectLocation);
		if (!chosenFolder.empty())
			newProjectLocation = chosenFolder;
	}

	ImGui::TextDisabled("The project will be created in: %s", (newProjectLocation / newProjectName).string().c_str());

	if (!newProjectError.empty())
		ImGui::TextColored(ErrorColor, "%s", newProjectError.c_str());

	if (ImGui::Button("Create"))
		CreateNewProject();

	ImGui::SameLine();
	if (ImGui::Button("Cancel"))
		ImGui::CloseCurrentPopup();

	ImGui::EndPopup();
}

void ProjectLauncherLayer::CreateNewProject()
{
	fs::path createdProject;

	if (!ProjectManager::CreateProject(newProjectLocation, newProjectName, createdProject, newProjectError))
		return;

	ImGui::CloseCurrentPopup();
	OpenProject(createdProject);
}

void ProjectLauncherLayer::OpenProjectFromFolderDialog()
{
	const fs::path startFolder = recentProjects.empty() ? newProjectLocation : recentProjects.front().directory.parent_path();
	const fs::path chosenFolder = ProjectManager::BrowseForFolder(startFolder);

	if (chosenFolder.empty())
		return;

	if (!ProjectManager::IsProjectDirectory(chosenFolder))
	{
		statusMessage = "This folder is not a Sunta project: it has no 'res' folder inside.";
		return;
	}

	OpenProject(chosenFolder);
}

void ProjectLauncherLayer::OpenProject(const fs::path& projectDirectory)
{
	EditorSettings::AddRecentProject(projectDirectory);

	if (!ProcessLauncher::StartEditorWithProject(projectDirectory))
	{
		statusMessage = "Couldn't start the editor, see the log.";
		return;
	}

	application.Close();
}

}
