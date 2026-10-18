#pragma once
#include <filesystem>

namespace Sunta
{

// "SuntaEditor --build-game <project folder> [--development]": builds the game without opening any window,
// for scripts and the CI (see GameBuilder for what a build is)
// The only thing it needs is the project folder, no editor, window or graphics card is started
class HeadlessGameBuild
{
public:
	// Returns the exit code of the program: 0 when the game was built, 1 when not (the reason is in the log)
	static int Run(const std::filesystem::path& projectDirectory, bool isDevelopmentBuild);
};

}
