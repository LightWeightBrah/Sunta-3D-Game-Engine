#include "Core/SuntaPreCompiled.h"

#include "MacPlatform.h"

namespace Sunta
{

bool MacPlatform::OpenInExplorer(const std::string& path)
{
	std::string command = "open " + path;
	return std::system(command.c_str()) == 0;
}

}