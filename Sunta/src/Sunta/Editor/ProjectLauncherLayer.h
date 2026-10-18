#pragma once
#include <filesystem>
#include <string>
#include <vector>

#include "Sunta/Core/Layer.h"

namespace Sunta
{

class Application;

// The window shown before the editor: choosing, creating and opening game projects
// Opening a project starts the editor as a new process with the project folder and closes the launcher
class ProjectLauncherLayer : public Layer
{
public:
	explicit ProjectLauncherLayer(Application& application);

	void OnAttach() override;
	void OnDetach() override;
	void OnProcessInput() override;
	void OnRender() override;

	// Example project offered next to the recent ones (the one from the repository). Optional
	static void SetSampleProjectDirectory(const std::filesystem::path& directory);

private:
	struct ProjectEntry
	{
		std::filesystem::path directory;
		std::string           name;
		std::string           pathText;
		bool                  exists = false;
	};

	Application& application;

	std::vector<ProjectEntry> recentProjects;
	std::string               statusMessage;

	bool                  shouldOpenNewProjectPopup = false;
	std::string           newProjectName            = "NewGame";
	std::filesystem::path newProjectLocation;
	std::string           newProjectError;

	void RefreshRecentProjects();
	ProjectEntry CreateEntry(const std::filesystem::path& directory) const;

	void DrawLauncherWindow();
	void DrawTitle();
	void DrawRecentProjects();
	void DrawRecentProjectRow(const ProjectEntry& project, std::filesystem::path& outProjectToRemove);
	void DrawActions();
	void DrawNewProjectPopup();

	void OpenProjectFromFolderDialog();
	void OpenProject(const std::filesystem::path& projectDirectory);
	void CreateNewProject();
};

}
