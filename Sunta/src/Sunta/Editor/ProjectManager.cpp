#include "Core/SuntaPreCompiled.h"
#include "ProjectManager.h"

#include <nfd.hpp>

#include "Core/GameConfig.h"
#include "Core/Log.h"
#include "Core/VirtualFileSystem.h"

namespace Sunta
{

namespace
{
	namespace fs = std::filesystem;

	constexpr const char* ResourcesFolderName = "res";
	constexpr const char* ConfigFileName      = "game.config";
	// The same folders as the resources of the engine (Sunta/res). The "App" folder (game icon) is created when an icon is chosen
	const std::vector<const char*> NewProjectFolders = { "Fonts", "Materials", "Models", "Scenes", "Scripts", "Shaders", "Textures" };

	bool isSwitchProjectRequested = false;

	// Project name becomes a folder name, so characters Windows doesn't allow are not accepted
	bool IsValidFolderName(const std::string& name)
	{
		const std::string forbiddenCharacters = "<>:\"/\\|?*";
		return !name.empty() && name.find_first_of(forbiddenCharacters) == std::string::npos && name != "." && name != "..";
	}
}

bool ProjectManager::IsProjectDirectory(const fs::path& directory)
{
	std::error_code error;
	return fs::is_directory(directory / ResourcesFolderName, error);
}

std::string ProjectManager::ReadProjectName(const fs::path& projectDirectory)
{
	const fs::path configPath = projectDirectory / ResourcesFolderName / ConfigFileName;

	std::error_code error;
	if (fs::exists(configPath, error))
	{
		// A path without "@" is not changed by the virtual file system, so it is read as it is
		GameConfig gameConfig;
		if (GameConfig::TryLoad(configPath.string(), gameConfig) && !gameConfig.name.empty())
			return gameConfig.name;
	}

	return projectDirectory.filename().string();
}

bool ProjectManager::CreateProject(const fs::path& parentDirectory, const std::string& projectName,
								   fs::path& outProjectDirectory, std::string& outError)
{
	if (!IsValidFolderName(projectName))
	{
		outError = "The name is empty or has characters a folder name can't have: < > : \" / \\ | ? *";
		return false;
	}

	if (!fs::is_directory(parentDirectory))
	{
		outError = "The location folder doesn't exist.";
		return false;
	}

	const fs::path projectDirectory = parentDirectory / projectName;

	std::error_code error;
	if (fs::exists(projectDirectory, error))
	{
		outError = "A folder with this name already exists in the location.";
		return false;
	}

	for (const char* folderName : NewProjectFolders)
	{
		fs::create_directories(projectDirectory / ResourcesFolderName / folderName, error);
		if (error)
		{
			outError = "Couldn't create the project folders: " + error.message();
			return false;
		}
	}

	GameConfig gameConfig;
	gameConfig.name = projectName;

	const fs::path configPath = projectDirectory / ResourcesFolderName / ConfigFileName;
	if (!GameConfig::TrySave(configPath.string(), gameConfig))
	{
		outError = "Couldn't write game.config.";
		return false;
	}

	SUNTA_ENGINE_LOG_INFO("ProjectManager: Created project '{0}' in '{1}'", projectName, projectDirectory.string());

	outProjectDirectory = projectDirectory;
	return true;
}

fs::path ProjectManager::GetCurrentProjectDirectory()
{
	// "@game" points to <project>/res
	return VirtualFileSystem::GetMountPath("@game").parent_path();
}

fs::path ProjectManager::BrowseForFolder(const fs::path& startDirectory)
{
	std::string startPath;

	std::error_code error;
	if (!startDirectory.empty() && fs::is_directory(startDirectory, error))
		startPath = fs::path(startDirectory).make_preferred().string();

	NFD::UniquePath chosenPath;
	const nfdresult_t result = NFD::PickFolder(chosenPath, startPath.empty() ? nullptr : startPath.c_str());

	if (result != NFD_OKAY)
		return fs::path();

	return fs::u8path(chosenPath.get());
}

void ProjectManager::RequestSwitchProject()
{
	isSwitchProjectRequested = true;
}

bool ProjectManager::ConsumeSwitchProjectRequest()
{
	const bool wasRequested = isSwitchProjectRequested;
	isSwitchProjectRequested = false;
	return wasRequested;
}

}
