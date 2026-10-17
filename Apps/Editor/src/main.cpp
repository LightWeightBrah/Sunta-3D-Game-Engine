#include <filesystem>
#include <string>
#include <vector>

#include <Sunta/Core/Application.h>
#include <Sunta/Editor/EditorLayer.h>
#include <Sunta/Editor/EditorSettings.h>
#include <Sunta/Editor/ProcessLauncher.h>
#include <Sunta/Editor/ProjectLauncherLayer.h>
#include <Sunta/Editor/ProjectManager.h>

// Usage:
//   SuntaEditor                  shows the project launcher (or opens the last project, when the launcher is set to do so)
//   SuntaEditor <project folder> opens the project at once
//   SuntaEditor --launcher       always shows the project launcher
namespace
{
	constexpr unsigned int LauncherWindowWidth  = 1100;
	constexpr unsigned int LauncherWindowHeight = 680;

	struct LaunchOptions
	{
		std::filesystem::path projectDirectory;
		bool                  forceLauncher = false;
	};

	LaunchOptions ParseLaunchOptions(const std::vector<std::string>& arguments)
	{
		LaunchOptions options;

		// arguments[0] is the program itself
		for (size_t index = 1; index < arguments.size(); ++index)
		{
			if (arguments[index] == Sunta::ProcessLauncher::LauncherArgument)
				options.forceLauncher = true;
			else
				options.projectDirectory = std::filesystem::u8path(arguments[index]);
		}

		return options;
	}

	// Project to open without asking, empty when the user has to choose
	std::filesystem::path FindProjectToOpen(const LaunchOptions& options)
	{
		using Sunta::ProjectManager;

		if (!options.projectDirectory.empty() && ProjectManager::IsProjectDirectory(options.projectDirectory))
			return options.projectDirectory;

		if (options.forceLauncher || !Sunta::EditorSettings::ShouldOpenLastProjectOnStartup())
			return std::filesystem::path();

		for (const std::filesystem::path& recentProject : Sunta::EditorSettings::GetRecentProjects())
		{
			if (ProjectManager::IsProjectDirectory(recentProject))
				return recentProject;
		}

		return std::filesystem::path();
	}

	void RunEditor(const std::filesystem::path& projectDirectory)
	{
		Sunta::ApplicationSpecification specification;
		specification.windowTitle      = "Sunta Editor";
		specification.assetSource      = Sunta::AssetSource::SourceTree;
		specification.projectDirectory = projectDirectory;

		Sunta::Application application(specification);
		application.PushLayer<Sunta::EditorLayer>();
		application.Run();
	}

	void RunProjectLauncher()
	{
		const std::filesystem::path sampleProject = SUNTA_DEFAULT_PROJECT_DIR;
		Sunta::ProjectLauncherLayer::SetSampleProjectDirectory(sampleProject);

		Sunta::ApplicationSpecification specification;
		specification.windowTitle      = "Sunta Engine";
		specification.windowWidth      = LauncherWindowWidth;
		specification.windowHeight     = LauncherWindowHeight;
		specification.assetSource      = Sunta::AssetSource::SourceTree;
		specification.projectDirectory = sampleProject;   // the engine needs some project folder to start, nothing of it is used
		specification.loadEngineResources = false;        // the launcher draws only ImGui, the editor loads the resources when a project opens

		Sunta::Application application(specification);
		application.PushLayer<Sunta::ProjectLauncherLayer>();
		application.Run();
	}
}

int main(int argc, char** argv)
{
	const LaunchOptions options = ParseLaunchOptions(Sunta::ProcessLauncher::GetCommandLineArguments(argc, argv));
	const std::filesystem::path projectToOpen = FindProjectToOpen(options);

	if (projectToOpen.empty())
		RunProjectLauncher();
	else
		RunEditor(projectToOpen);

	return 0;
}
