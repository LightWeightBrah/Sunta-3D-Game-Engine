#include "Core/SuntaPreCompiled.h"

#include "LinuxPlatform.h"

namespace Sunta
{

namespace
{

// The text as one argument of a shell command: in single quotes, a single quote inside becomes '\''
std::string QuoteForShell(const std::string& text)
{
	std::string quoted = "'";

	for (const char character : text)
	{
		if (character == '\'')
			quoted += "'\\''";
		else
			quoted += character;
	}

	return quoted + "'";
}

}

bool LinuxPlatform::OpenInExplorer(const std::string & path)
{
	std::filesystem::path absolutePath = std::filesystem::absolute(path);

	// if path is file, open it's parent folder (otherwise it would open file in external app)
	if (!std::filesystem::is_directory(absolutePath))
		absolutePath = absolutePath.parent_path();

	// & in linux commands means do this command asynchronically in background
	std::string command = "xdg-open \"" + absolutePath.string() + "\" &";
	return std::system(command.c_str()) == 0;
}

bool LinuxPlatform::OpenFileExternally(const std::string& path)
{
	std::filesystem::path absolutePath = std::filesystem::absolute(path);

	// & in linux commands means do this command asynchronically in background
	std::string command = "xdg-open \"" + absolutePath.string() + "\" &";
	return std::system(command.c_str()) == 0;
}

void LinuxPlatform::ShowErrorMessage(const std::string& title, const std::string& message)
{
	// For the user who started the game from a terminal
	std::cerr << title << ": " << message << std::endl;

	// The first of these that is installed shows the message
	const std::string quotedTitle   = QuoteForShell(title);
	const std::string quotedMessage = QuoteForShell(message);

	const std::string command =
		"zenity --error --no-markup --title=" + quotedTitle + " --text=" + quotedMessage + " 2>/dev/null"
		" || kdialog --error " + quotedMessage + " --title " + quotedTitle + " 2>/dev/null"
		" || xmessage -center " + quotedMessage + " 2>/dev/null";

	std::system(command.c_str());
}

}