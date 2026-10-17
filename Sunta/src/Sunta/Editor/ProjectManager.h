#pragma once
#include <filesystem>
#include <string>

namespace Sunta
{

// Everything about game projects as folders: checking them, creating new ones, browsing for them
// A project is a folder with "res/" inside, "res/game.config" holds its settings
class ProjectManager
{
public:
	// True when the folder has a "res" folder
	static bool IsProjectDirectory(const std::filesystem::path& directory);

	// Name from game.config, the folder name when there is no (readable) config
	static std::string ReadProjectName(const std::filesystem::path& projectDirectory);

	// Creates <parentDirectory>/<projectName>/res with Scenes, Scripts, App and game.config
	// On failure returns false and fills outError with a message for the user
	static bool CreateProject(const std::filesystem::path& parentDirectory, const std::string& projectName,
							  std::filesystem::path& outProjectDirectory, std::string& outError);

	// The project the editor has open right now
	static std::filesystem::path GetCurrentProjectDirectory();

	// System dialog for choosing a folder. Returns an empty path when the user cancels
	static std::filesystem::path BrowseForFolder(const std::filesystem::path& startDirectory);

	// Switching projects restarts the editor (the engine keeps one project per run)
	// The menu only sets the request, EditorLayer carries it out
	static void RequestSwitchProject();
	static bool ConsumeSwitchProjectRequest();
};

}
