#include "Core/SuntaPreCompiled.h"

#include "MacPlatform.h"

namespace Sunta
{

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

}