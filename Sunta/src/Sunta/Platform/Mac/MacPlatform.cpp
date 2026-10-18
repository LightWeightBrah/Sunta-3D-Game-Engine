#include "Core/SuntaPreCompiled.h"

#include "MacPlatform.h"

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

// The text inside an AppleScript string literal: backslash and quote get a backslash, a new line becomes \n
std::string EscapeForAppleScript(const std::string& text)
{
	std::string escaped;

	for (const char character : text)
	{
		if (character == '\\' || character == '"')
			escaped += '\\';

		if (character == '\n')
			escaped += "\\n";
		else
			escaped += character;
	}

	return escaped;
}

}

bool MacPlatform::OpenInExplorer(const std::string& path)
{
	std::filesystem::path absolutePath = std::filesystem::absolute(path);

	std::string command = "open -R \"" + absolutePath.string() + "\"";
	return std::system(command.c_str()) == 0;
}

bool MacPlatform::OpenFileExternally(const std::string& path)
{
	std::filesystem::path absolutePath = std::filesystem::absolute(path);

	std::string command = "open \"" + absolutePath.string() + "\"";
	return std::system(command.c_str()) == 0;
}

void MacPlatform::ShowErrorMessage(const std::string& title, const std::string& message)
{
	// For the user who started the game from a terminal
	std::cerr << title << ": " << message << std::endl;

	const std::string script = "display alert \"" + EscapeForAppleScript(title) + "\" message \"" + EscapeForAppleScript(message) + "\" as critical";
	const std::string command = "osascript -e " + QuoteForShell(script);

	std::system(command.c_str());
}

}