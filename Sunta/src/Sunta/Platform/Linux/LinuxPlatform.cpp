#include "Core/SuntaPreCompiled.h"

#include "LinuxPlatform.h"

namespace Sunta
{

bool LinuxPlatform::OpenInExplorer(const std::string & path)
{
	std::string command = "xdg-open " + path;
	return std::system(command.c_str()) == 0;
}

}