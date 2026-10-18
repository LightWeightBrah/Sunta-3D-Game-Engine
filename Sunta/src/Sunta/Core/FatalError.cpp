#include "SuntaPreCompiled.h"
#include "FatalError.h"

#include "Log.h"
#include "LogFile.h"
#include "Platform.h"
#include "VirtualFileSystem.h"

namespace Sunta
{

void FatalError::Report(const std::string& message)
{
	SUNTA_ENGINE_LOG_ERROR("Fatal error: {0}", message);

	std::string text = message;

	const std::filesystem::path logFilePath = LogFile::GetPath();
	if (!logFilePath.empty())
		text += "\n\nMore details are in the log file:\n" + logFilePath.string();

	// The name of the program is the title, so a player sees which game it is
	const std::string title = VirtualFileSystem::GetExecutablePath().stem().string();

	if (Platform::IsInitialized())
		Platform::Get().ShowErrorMessage(title, text);

	std::exit(EXIT_FAILURE);
}

}
