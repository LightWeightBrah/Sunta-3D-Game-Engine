#pragma once
#include <filesystem>
#include <string>

#include "Sunta/Core/GameConfig.h"
#include "Sunta/Editor/BuildProgress.h"

namespace Sunta
{

// What a build ended with
struct BuildResult
{
	bool                  isSuccess = false;
	std::string           errorMessage;    // the reason, when it failed
	std::filesystem::path outputDirectory; // the folder of the game, when it succeeded
};

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
	// Builds the game and says how it went. Doesn't open anything, so it can run on another thread (see GameBuildJob):
	// the log is safe to use from there, the progress (optional) can be read by another thread while the build runs
	static BuildResult BuildGame(bool isDevelopmentBuild, BuildProgress* progress = nullptr);

	// False when the Release game executable was not compiled yet
	static bool IsRuntimeAvailable();

	// Where the Release game executable should lie (may not exist)
	static std::filesystem::path GetRuntimeTemplatePath();

private:
	// The steps of a build: every one of them throws when it fails, the message becomes BuildResult::errorMessage
	static std::filesystem::path RunBuild(bool isDevelopmentBuild, BuildProgress& progress);

	static GameConfig            LoadGameConfig();
	static std::string           GetGameName(const GameConfig& gameConfig);
	static std::filesystem::path GetProjectDirectory();
	static std::filesystem::path GetOutputDirectory(const std::string& gameName, bool isDevelopmentBuild);

	// Folder with the executable and "res": the output folder, on macOS the inside of the .app
	static std::filesystem::path GetPackageDirectory(const std::filesystem::path& outputDirectory, const std::string& gameName);
	static std::filesystem::path GetRuntimeDirectory();

	static void PrepareOutputDirectory(const std::filesystem::path& outputDirectory, const std::filesystem::path& packageDirectory);
	static void CopyRuntimeExecutable (const std::filesystem::path& runtimePath, const std::filesystem::path& gameExecutablePath);
	static void CopyEngineResources   (const std::filesystem::path& packageDirectory, BuildProgress& progress);
	static void CopyGameResources     (const std::filesystem::path& packageDirectory, BuildProgress& progress);

	// The game reads this copy of game.config, not the one of the project
	static void WriteBuiltGameConfig  (const std::filesystem::path& packageDirectory, GameConfig gameConfig, bool isDevelopmentBuild);

	// Not fatal: without the icon the game just has the icon of the engine
	static void ApplyGameIcon(const std::filesystem::path& gameExecutablePath, const GameConfig& gameConfig);
	static void WriteLinuxDesktopEntry(const std::filesystem::path& outputDirectory, const std::string& gameName);
	static void WriteMacBundleFiles   (const std::filesystem::path& outputDirectory, const std::string& gameName, const GameConfig& gameConfig);

	static void   CopyDirectory(const std::filesystem::path& source, const std::filesystem::path& destination, bool skipEditorOnlyResources, BuildProgress& progress);
	static size_t CountFilesToCopy(const std::filesystem::path& source, bool skipEditorOnlyResources);
	static bool IsEditorOnlyResource(const std::filesystem::path& pathInsideEngineResources);
};

}
