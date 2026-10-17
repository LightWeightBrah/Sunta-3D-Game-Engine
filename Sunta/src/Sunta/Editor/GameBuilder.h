#pragma once
#include <filesystem>
#include <string>

#include "Sunta/Core/GameConfig.h"

namespace Sunta
{

// Which program the game is made from: the game executable compiled in Debug (slow, has debug checks) or in Release
enum class BuildConfiguration
{
	Debug,
	Release
};

// Packages the opened game project into a folder that can be given to players, no compiler needed:
//   <project>/Build/<GameName>/<GameName>.exe   (copy of SuntaRuntime.exe, with the icon of the game)
//   <project>/Build/<GameName>/res/Sunta        (engine resources without editor-only files)
//   <project>/Build/<GameName>/res/Game         (resources of the project: scenes, scripts, models...)
// A Debug build goes to <GameName>_Debug, so it can't be mistaken for the version to share
//
// SuntaRuntime is compiled by CMake together with the editor, in the same configuration as the editor.
// The other configuration is available when it was compiled before (its folder lies next to the folder of the editor)
class GameBuilder
{
public:
	// Returns true and opens the result folder when the build succeeded
	static bool BuildGame(BuildConfiguration configuration);

	// Builds in the same configuration as this editor
	static bool BuildGame();

	static bool IsConfigurationAvailable(BuildConfiguration configuration);

	// Configuration this editor was compiled in
	static BuildConfiguration GetEditorConfiguration();

	static const char* GetConfigurationName(BuildConfiguration configuration);
	static BuildConfiguration ParseConfiguration(const std::string& name, BuildConfiguration fallback);

	// Where the game executable of the configuration should lie (may not exist)
	static std::filesystem::path GetRuntimeTemplatePath(BuildConfiguration configuration);

private:
	static GameConfig            LoadGameConfig();
	static std::string           GetGameName(const GameConfig& gameConfig);
	static std::filesystem::path GetProjectDirectory();
	static std::filesystem::path GetOutputDirectory(const std::string& gameName, BuildConfiguration configuration);
	static std::filesystem::path GetRuntimeDirectory(BuildConfiguration configuration);

	static bool PrepareOutputDirectory(const std::filesystem::path& outputDirectory);
	static bool CopyRuntimeExecutable  (const std::filesystem::path& runtimePath, const std::filesystem::path& gameExecutablePath);
	static bool CopyEngineResources    (const std::filesystem::path& outputDirectory);
	static bool CopyGameResources      (const std::filesystem::path& outputDirectory);

	// Not fatal: without the icon the game just has the icon of the engine
	static void ApplyGameIcon(const std::filesystem::path& gameExecutablePath, const GameConfig& gameConfig);
	static void WriteLinuxDesktopEntry(const std::filesystem::path& outputDirectory, const std::string& gameName);

	static bool CopyDirectory(const std::filesystem::path& source, const std::filesystem::path& destination, bool skipEditorOnlyResources);
	static bool IsEditorOnlyResource(const std::filesystem::path& pathInsideEngineResources);
};

}
