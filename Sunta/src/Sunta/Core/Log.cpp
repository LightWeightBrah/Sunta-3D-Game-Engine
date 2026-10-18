#include "SuntaPreCompiled.h"

#include "Log.h"

namespace Sunta {

std::unique_ptr<Logger> Log::engineLogger;
std::unique_ptr<Logger> Log::gameLogger;

namespace
{

// A folder without write access (e.g. a game started straight from a zip file) can't hold the log,
// so the log goes to the temporary folder of the system instead
// Returns the file that was opened, empty when no file could be opened
std::filesystem::path OpenLogFile(const std::filesystem::path& logFilePath)
{
	if (logFilePath.empty())
		return {};

	if (LogFile::Open(logFilePath))
		return logFilePath;

	std::error_code errorCode;
	const std::filesystem::path fallbackPath = std::filesystem::temp_directory_path(errorCode) / logFilePath.filename();

	if (!errorCode && LogFile::Open(fallbackPath))
		return fallbackPath;

	return {};
}

}

void Log::Init(const std::filesystem::path& logFilePath)
{
	Terminal::Init();

	const std::filesystem::path openedLogFilePath = OpenLogFile(logFilePath);

	engineLogger = std::make_unique<Logger>("SUNTA");

	// Lua print() messages are written by this one, blue stands out from the engine messages
	gameLogger   = std::make_unique<Logger>("GAME", Terminal::Color::Blue);

	if (logFilePath.empty() || openedLogFilePath == logFilePath)
		return;

	if (openedLogFilePath.empty())
		engineLogger->Warning("Log: Couldn't create the log file '{0}', this run has no log file", logFilePath.string());
	else
		engineLogger->Warning("Log: Couldn't create the log file '{0}', the log is in '{1}'", logFilePath.string(), openedLogFilePath.string());
}

}