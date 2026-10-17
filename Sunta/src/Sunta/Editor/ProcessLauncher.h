#pragma once
#include <filesystem>
#include <string>
#include <vector>

namespace Sunta
{

// Starts another copy of the editor as a separate process (used to open a project or the project launcher)
class ProcessLauncher
{
public:
	// "SuntaEditor --launcher" always shows the project launcher
	static constexpr const char* LauncherArgument = "--launcher";

	// Command line arguments as UTF-8 (on Windows argv is in the system code page, which breaks non-English paths)
	// The first element is the program, like in argv
	static std::vector<std::string> GetCommandLineArguments(int argc, char** argv);

	static bool StartEditorWithProject(const std::filesystem::path& projectDirectory);
	static bool StartEditorLauncher();

private:
	static bool StartDetached(const std::filesystem::path& executablePath, const std::vector<std::string>& arguments);
};

}
