#pragma once
#include <string>

namespace Sunta
{

// Something the program can't go on without (no window, no game files...)
// Without this a player of a Release game only sees that the game doesn't open, because it has no console window
class FatalError
{
public:
	// Writes the message to the log, shows it in a window (with the place of the log file) and ends the program
	// Call it only after Log::Init(), the window needs Platform::Init() as well (it is skipped when that didn't happen yet)
	[[noreturn]] static void Report(const std::string& message);
};

}
