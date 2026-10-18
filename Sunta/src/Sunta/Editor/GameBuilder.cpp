#include "Core/SuntaPreCompiled.h"
#include "GameBuilder.h"

#include "ExecutableIcon.h"
#include "MacAppBundle.h"
#include "Core/EngineAssets.h"
#include "Core/Log.h"
#include "Core/Platform.h"
#include "Core/VirtualFileSystem.h"

namespace Sunta
{

namespace
{

namespace fs = std::filesystem;

constexpr const char* RuntimeExecutableName = "SuntaRuntime";
constexpr const char* BuildFolderName       = "Build";
constexpr const char* ReleaseFolderPrefix   = "Release";

// Steps of a build that are not copying resource files: the game executable, game.config, the icon
constexpr size_t NonResourceBuildSteps = 3;

// Resources only the editor uses (relative to Sunta/res), the shipped game doesn't need them
const std::vector<std::string> EditorOnlyResourceFolders = { "Textures/Icons/", "Textures/ui/" };

std::string GetExecutableSuffix()
{
#if defined(SUNTA_PLATFORM_WINDOWS)
	return ".exe";
#else
	return "";
#endif
}

// A build step failed, the message is shown to the user
class BuildError : public std::runtime_error
{
public:
	explicit BuildError(const std::string& message) : std::runtime_error(message) {}
};

// A game without a start scene would open as an empty window
bool HasStartScene(const GameConfig& gameConfig)
{
	if (gameConfig.startScene.empty())
		return false;

	std::error_code error;
	return fs::exists(VirtualFileSystem::Resolve(gameConfig.startScene), error);
}

// The game name becomes a file and folder name, so characters Windows doesn't allow are replaced
std::string MakeFileNameSafe(std::string name)
{
	const std::string forbiddenCharacters = "<>:\"/\\|?*";

	for (char& character : name)
	{
		if (forbiddenCharacters.find(character) != std::string::npos)
			character = '_';
	}

	return name;
}

}

BuildResult GameBuilder::BuildGame(bool isDevelopmentBuild, BuildProgress* progress)
{
	BuildProgress ignoredProgress;
	BuildProgress& buildProgress = progress ? *progress : ignoredProgress;

	BuildResult result;

	try
	{
		result.outputDirectory = RunBuild(isDevelopmentBuild, buildProgress);
		result.isSuccess       = true;
	}
	catch (const std::exception& error)
	{
		result.errorMessage = error.what();
		SUNTA_ENGINE_LOG_ERROR("GameBuilder: Build FAILED: {0}", result.errorMessage);
	}

	return result;
}

fs::path GameBuilder::RunBuild(bool isDevelopmentBuild, BuildProgress& progress)
{
	const GameConfig gameConfig      = LoadGameConfig();
	const std::string gameName       = GetGameName(gameConfig);
	const fs::path   outputDirectory = GetOutputDirectory(gameName, isDevelopmentBuild);
	const fs::path   packageDirectory = GetPackageDirectory(outputDirectory, gameName);
	const fs::path   runtimePath     = GetRuntimeTemplatePath();
	const fs::path   gameExecutable  = packageDirectory / (gameName + GetExecutableSuffix());

	SUNTA_ENGINE_LOG_INFO("GameBuilder: Building '{0}' ({1}) to '{2}'", gameName, isDevelopmentBuild ? "development build" : "release build", outputDirectory.string());

	if (!HasStartScene(gameConfig))
		throw BuildError("The start scene '" + gameConfig.startScene + "' doesn't exist. Save a scene and choose it as the start scene in Project Settings.");

	if (!IsRuntimeAvailable())
		throw BuildError("There is no Release game executable (expected '" + runtimePath.string() + "'). Compile the SuntaRuntime target in the Release configuration first.");

	const size_t resourceFiles = CountFilesToCopy(VirtualFileSystem::GetMountPath("@engine"), true)
							   + CountFilesToCopy(VirtualFileSystem::GetMountPath("@game"), false);
	progress.Start(resourceFiles + NonResourceBuildSteps);

	progress.SetStep("Preparing the output folder");
	PrepareOutputDirectory(outputDirectory, packageDirectory);

	progress.SetStep("Copying the game executable");
	CopyRuntimeExecutable(runtimePath, gameExecutable);
	progress.Advance();

	progress.SetStep("Copying engine resources");
	CopyEngineResources(packageDirectory, progress);

	progress.SetStep("Copying game resources");
	CopyGameResources(packageDirectory, progress);

	progress.SetStep("Writing game.config");
	WriteBuiltGameConfig(packageDirectory, gameConfig, isDevelopmentBuild);
	progress.Advance();

	progress.SetStep("Setting the icon");
	ApplyGameIcon(gameExecutable, gameConfig);
	WriteLinuxDesktopEntry(outputDirectory, gameName);
	WriteMacBundleFiles(outputDirectory, gameName, gameConfig);
	progress.Advance();

	SUNTA_ENGINE_LOG_INFO("GameBuilder: Build of '{0}' finished", gameName);
	return outputDirectory;
}

bool GameBuilder::IsRuntimeAvailable()
{
	const fs::path runtimePath = GetRuntimeTemplatePath();

	std::error_code error;
	return !runtimePath.empty() && fs::exists(runtimePath, error);
}

GameConfig GameBuilder::LoadGameConfig()
{
	// Without a readable config the defaults of GameConfig stay
	GameConfig gameConfig;
	GameConfig::TryLoad(GameConfig::DefaultPath, gameConfig);

	return gameConfig;
}

std::string GameBuilder::GetGameName(const GameConfig& gameConfig)
{
	return MakeFileNameSafe(gameConfig.name);
}

fs::path GameBuilder::GetProjectDirectory()
{
	// "@game" points to <project>/res
	return VirtualFileSystem::GetMountPath("@game").parent_path();
}

fs::path GameBuilder::GetOutputDirectory(const std::string& gameName, bool isDevelopmentBuild)
{
	// "<GameName>_Release" and "<GameName>_Development", so the two builds are never mixed up
	const std::string folderName = gameName + (isDevelopmentBuild ? "_Development" : "_Release");
	return GetProjectDirectory() / BuildFolderName / folderName;
}

fs::path GameBuilder::GetPackageDirectory(const fs::path& outputDirectory, [[maybe_unused]] const std::string& gameName)
{
#if defined(SUNTA_PLATFORM_MAC)
	// The game lives inside <GameName>.app, the executable and "res" lie next to each other in Contents/MacOS
	return MacAppBundle::GetExecutableDirectory(outputDirectory / (gameName + ".app"));
#else
	return outputDirectory;
#endif
}

fs::path GameBuilder::GetRuntimeDirectory()
{
	const fs::path editorDirectory = VirtualFileSystem::GetExecutableDirectory();

	// SUNTA_DEBUG is defined by CMake for the Debug configuration only. A Release editor has the game executable next to itself
#if !defined(SUNTA_DEBUG)
	return editorDirectory;
#else
	// CMake names the folders "<Configuration>-<System>-<Processor>" e.g. "Release-Windows-AMD64"
	// The Release one lies next to ours, with only the first part of the name changed
	const std::string folderName = editorDirectory.filename().string();
	const size_t      separator  = folderName.find('-');

	if (separator == std::string::npos)
		return fs::path();

	return editorDirectory.parent_path() / (std::string(ReleaseFolderPrefix) + folderName.substr(separator));
#endif
}

fs::path GameBuilder::GetRuntimeTemplatePath()
{
	const fs::path runtimeDirectory = GetRuntimeDirectory();
	if (runtimeDirectory.empty())
		return fs::path();

	return runtimeDirectory / (std::string(RuntimeExecutableName) + GetExecutableSuffix());
}

void GameBuilder::PrepareOutputDirectory(const fs::path& outputDirectory, const fs::path& packageDirectory)
{
	std::error_code error;

	// Removes files of the previous build, so deleted assets don't stay in the package
	fs::remove_all(outputDirectory, error);
	fs::create_directories(packageDirectory, error);

	if (error)
		throw BuildError("Couldn't prepare the folder '" + outputDirectory.string() + "': " + error.message());
}

void GameBuilder::CopyRuntimeExecutable(const fs::path& runtimePath, const fs::path& gameExecutablePath)
{
	std::error_code error;
	fs::copy_file(runtimePath, gameExecutablePath, fs::copy_options::overwrite_existing, error);

	if (error)
		throw BuildError("Couldn't copy the game executable: " + error.message());
}

void GameBuilder::CopyEngineResources(const fs::path& packageDirectory, BuildProgress& progress)
{
	const fs::path source      = VirtualFileSystem::GetMountPath("@engine");
	const fs::path destination = packageDirectory / "res" / "Sunta";

	CopyDirectory(source, destination, true, progress);
}

void GameBuilder::CopyGameResources(const fs::path& packageDirectory, BuildProgress& progress)
{
	const fs::path source      = VirtualFileSystem::GetMountPath("@game");
	const fs::path destination = packageDirectory / "res" / "Game";

	CopyDirectory(source, destination, false, progress);
}

void GameBuilder::WriteBuiltGameConfig(const fs::path& packageDirectory, GameConfig gameConfig, bool isDevelopmentBuild)
{
	gameConfig.developerMode = isDevelopmentBuild;

	const fs::path configPath = packageDirectory / "res" / "Game" / fs::path(GameConfig::DefaultPath).filename();

	if (!GameConfig::TrySaveToFile(configPath, gameConfig))
		throw BuildError("Couldn't write '" + configPath.string() + "'");
}

void GameBuilder::ApplyGameIcon([[maybe_unused]] const fs::path& gameExecutablePath, const GameConfig& gameConfig)
{
	const fs::path iconPath = VirtualFileSystem::Resolve(gameConfig.windowIcon);

	// No icon in the project: the game keeps the icon of the engine
	std::error_code error;
	if (!fs::exists(iconPath, error))
		return;

#if defined(SUNTA_PLATFORM_WINDOWS)
	if (!ExecutableIcon::Apply(gameExecutablePath, iconPath))
		SUNTA_ENGINE_LOG_WARNING("GameBuilder: The game was built, but it has the icon of the engine (changing the icon failed)");
#endif
}

void GameBuilder::WriteLinuxDesktopEntry(const fs::path& outputDirectory, const std::string& gameName)
{
#if defined(SUNTA_PLATFORM_LINUX)
	// Linux has no icon inside an executable, the file manager takes it from a .desktop file instead
	std::ofstream desktopFile(outputDirectory / (gameName + ".desktop"));
	if (!desktopFile.is_open())
		return;

	desktopFile << "[Desktop Entry]\n"
				<< "Type=Application\n"
				<< "Name=" << gameName << "\n"
				<< "Comment=Built with Sunta Engine\n"
				<< "Exec=bash -c 'cd \"$(dirname \"%k\")\" && ./\"" << gameName << "\"'\n"
				<< "Icon=./res/Game/App/window_icon.png\n"
				<< "Terminal=false\n"
				<< "Categories=Game;\n";
#endif
}

void GameBuilder::WriteMacBundleFiles([[maybe_unused]] const fs::path& outputDirectory, [[maybe_unused]] const std::string& gameName,
									  [[maybe_unused]] const GameConfig& gameConfig)
{
#if defined(SUNTA_PLATFORM_MAC)
	// macOS takes the icon from the .icns file in the bundle, it is made from the PNG of the project
	const fs::path bundleDirectory = outputDirectory / (gameName + ".app");

	fs::path iconPath = VirtualFileSystem::Resolve(gameConfig.windowIcon);

	std::error_code error;
	if (!fs::exists(iconPath, error))
		iconPath = VirtualFileSystem::Resolve(EngineAssets::App::EngineLogoPath);

	const std::string iconFileName = MacAppBundle::WriteIcon(bundleDirectory, gameName, iconPath);
	if (iconFileName.empty())
		SUNTA_ENGINE_LOG_WARNING("GameBuilder: The game was built, but without an icon (making the .icns file failed)");

	if (!MacAppBundle::WriteInfoPlist(bundleDirectory, gameName, iconFileName))
		SUNTA_ENGINE_LOG_WARNING("GameBuilder: Couldn't write Info.plist, the .app may not start by double click");
#endif
}

void GameBuilder::CopyDirectory(const fs::path& source, const fs::path& destination, bool skipEditorOnlyResources, BuildProgress& progress)
{
	if (!fs::exists(source))
		throw BuildError("The folder '" + source.string() + "' doesn't exist");

	try
	{
		fs::create_directories(destination);

		for (const auto& entry : fs::recursive_directory_iterator(source))
		{
			const fs::path relativePath = fs::relative(entry.path(), source);

			if (skipEditorOnlyResources && IsEditorOnlyResource(relativePath))
				continue;

			const fs::path targetPath = destination / relativePath;

			if (entry.is_directory())
			{
				fs::create_directories(targetPath);
				continue;
			}

			fs::copy_file(entry.path(), targetPath, fs::copy_options::overwrite_existing);
			progress.Advance();
		}
	}
	catch (const fs::filesystem_error& error)
	{
		throw BuildError("Copying '" + source.string() + "' failed: " + error.what());
	}
}

// Counts what CopyDirectory will copy, so the progress bar knows the whole length of the build
size_t GameBuilder::CountFilesToCopy(const fs::path& source, bool skipEditorOnlyResources)
{
	size_t fileCount = 0;

	try
	{
		if (!fs::exists(source))
			return 0;

		for (const auto& entry : fs::recursive_directory_iterator(source))
		{
			const bool isSkipped = skipEditorOnlyResources && IsEditorOnlyResource(fs::relative(entry.path(), source));

			if (!entry.is_directory() && !isSkipped)
				++fileCount;
		}
	}
	catch (const fs::filesystem_error&)
	{
		// A folder that can't be read makes the copying fail with a message, the count is only for the progress bar
	}

	return fileCount;
}

bool GameBuilder::IsEditorOnlyResource(const fs::path& pathInsideEngineResources)
{
	// The trailing "/" makes the folder itself match too, not only the files inside it
	const std::string pathWithSlash = pathInsideEngineResources.generic_string() + "/";

	for (const std::string& editorOnlyFolder : EditorOnlyResourceFolders)
	{
		if (pathWithSlash.rfind(editorOnlyFolder, 0) == 0)
			return true;
	}

	return false;
}

}
