#include "SuntaPreCompiled.h"
#include "LogFile.h"

namespace Sunta
{

namespace
{

std::ofstream logStream;
std::filesystem::path logPath;

// Keeps the log of the last run (the one that may have ended with a crash) when the program is started again
void KeepPreviousLog(const std::filesystem::path& currentLogPath)
{
	std::error_code errorCode;

	if (!std::filesystem::exists(currentLogPath, errorCode))
		return;

	std::filesystem::path previousLogPath = currentLogPath;
	previousLogPath.replace_extension(".previous.log");

	std::filesystem::remove(previousLogPath, errorCode);
	std::filesystem::rename(currentLogPath, previousLogPath, errorCode);
}

}

bool LogFile::Open(const std::filesystem::path& path)
{
	KeepPreviousLog(path);

	logStream.open(path, std::ios::out | std::ios::trunc);

	if (!logStream.is_open())
		return false;

	logPath = path;
	return true;
}

void LogFile::Write(const std::string& line)
{
	if (!logStream.is_open())
		return;

	logStream << line << '\n';

	// The last lines before a crash are the ones that matter, so they can't wait in a buffer
	logStream.flush();
}

std::filesystem::path LogFile::GetPath()
{
	return logPath;
}

}
