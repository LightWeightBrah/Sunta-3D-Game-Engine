#pragma once
#include <filesystem>
#include <string>

namespace Sunta
{

// The log of the program written to a file, next to the console
// A shipped game on Windows has no console window, so this file is the only place its log can be read
class LogFile
{
public:
	// Starts writing every log line to the file. The log of the previous run is kept as "<name>.previous.log"
	// False when the file can't be created (e.g. a folder without write access): the program runs anyway
	static bool Open(const std::filesystem::path& path);

	// Does nothing when no file is open
	static void Write(const std::string& line);

	// The file the log is written to, empty when there is none
	static std::filesystem::path GetPath();
};

}
