#include "Core/SuntaPreCompiled.h"

#include "LinuxPlatform.h"

namespace Sunta
{

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

}