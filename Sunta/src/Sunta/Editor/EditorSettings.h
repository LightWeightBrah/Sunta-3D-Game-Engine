#pragma once
#include <filesystem>
#include <string>
#include <vector>

namespace Sunta
{

// Settings of the editor itself (not of a game), stored per user:
//   Windows: %APPDATA%/SuntaEngine/editor_settings.json
//   Linux:   ~/.config/SuntaEngine/editor_settings.json
//   Mac:     ~/Library/Application Support/SuntaEngine/editor_settings.json
// Loaded on first use, saved after every change
class EditorSettings
{
public:
	// Newest first, folders of the projects (UTF-8)
	static std::vector<std::filesystem::path> GetRecentProjects();
	static void AddRecentProject(const std::filesystem::path& projectDirectory);
	static void RemoveRecentProject(const std::filesystem::path& projectDirectory);

	// When true, the editor opens the last project at once instead of showing the project launcher
	static bool ShouldOpenLastProjectOnStartup();
	static void SetOpenLastProjectOnStartup(bool shouldOpen);

	// "Debug" or "Release", empty when the user never chose
	static std::string GetBuildConfiguration();
	static void SetBuildConfiguration(const std::string& configurationName);

	// Speed of the free camera in the editor (units per second), 0 when the user never changed it
	static float GetCameraSpeed();
	static void  SetCameraSpeed(float speed);
};

}
