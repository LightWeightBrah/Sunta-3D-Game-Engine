#pragma once
#include <filesystem>
#include <string>

#include "Sunta/Core/GameConfig.h"

namespace Sunta
{

// Packages the opened game project into a folder that can be given to players, no compiler needed:
//   <project>/Build/<GameName>_<Kind>/<GameName>.exe   (copy of SuntaRuntime.exe, with the icon of the game)
//   <project>/Build/<GameName>_<Kind>/res/Sunta        (engine resources without editor-only files)
//   <project>/Build/<GameName>_<Kind>/res/Game         (resources of the project: scenes, scripts, models...)
// <Kind> is Release (the game for players) or Development, so the two builds can't be mixed up
//
// The game is always made from the Release program. A development build is the same game, only its copy of game.config
// has "developer_mode": true, so it opens a console window and shows the log there (the project's own game.config stays clean)
//
// The icon of the game comes from one PNG (game.config, "window_icon") and is made for the system the editor runs on:
//   Windows: written into the .exe as icon resources
//   macOS:   the game is packed in <GameName>.app with a .icns icon (the executable and "res" are in <GameName>.app/Contents/MacOS)
//   Linux:   the PNG is used as it is, a <GameName>.desktop file points to it
//
// SuntaRuntime is compiled by CMake together with the editor. A Release editor has it next to itself, a Debug editor
// needs the Release one compiled too (its folder lies next to the folder of the editor)
class GameBuilder
{
public:
	// Returns true when the build succeeded, and opens the result folder then (unless shouldOpenResultFolder is false,
	// a build without a window has no use for it)
	static bool BuildGame(bool isDevelopmentBuild, bool shouldOpenResultFolder = true);

	// False when the Release game executable was not compiled yet
	static bool IsRuntimeAvailable();

	// Where the Release game executable should lie (may not exist)
	static std::filesystem::path GetRuntimeTemplatePath();

private:
	static GameConfig            LoadGameConfig();
	static std::string           GetGameName(const GameConfig& gameConfig);
	static std::filesystem::path GetProjectDirectory();
	static std::filesystem::path GetOutputDirectory(const std::string& gameName, bool isDevelopmentBuild);

	// Folder with the executable and "res": the output folder, on macOS the inside of the .app
	static std::filesystem::path GetPackageDirectory(const std::filesystem::path& outputDirectory, const std::string& gameName);
	static std::filesystem::path GetRuntimeDirectory();

	static bool PrepareOutputDirectory(const std::filesystem::path& outputDirectory, const std::filesystem::path& packageDirectory);
	static bool CopyRuntimeExecutable  (const std::filesystem::path& runtimePath, const std::filesystem::path& gameExecutablePath);
	static bool CopyEngineResources    (const std::filesystem::path& packageDirectory);
	static bool CopyGameResources      (const std::filesystem::path& packageDirectory);

	// The game reads this copy of game.config, not the one of the project
	static bool WriteBuiltGameConfig   (const std::filesystem::path& packageDirectory, GameConfig gameConfig, bool isDevelopmentBuild);

	// Not fatal: without the icon the game just has the icon of the engine
	static void ApplyGameIcon(const std::filesystem::path& gameExecutablePath, const GameConfig& gameConfig);
	static void WriteLinuxDesktopEntry(const std::filesystem::path& outputDirectory, const std::string& gameName);
	static void WriteMacBundleFiles   (const std::filesystem::path& outputDirectory, const std::string& gameName, const GameConfig& gameConfig);

	static bool CopyDirectory(const std::filesystem::path& source, const std::filesystem::path& destination, bool skipEditorOnlyResources);
	static bool IsEditorOnlyResource(const std::filesystem::path& pathInsideEngineResources);
};

}
