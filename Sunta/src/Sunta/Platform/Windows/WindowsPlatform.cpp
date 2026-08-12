#include "Core/SuntaPreCompiled.h"

#include "WindowsPlatform.h"

namespace Sunta
{

bool WindowsPlatform::OpenInExplorer(const std::string& path)
{
	HINSTANCE result = ShellExecuteA(NULL, "explore", path.c_str(), NULL, NULL, SW_SHOWNORMAL);
	return (intptr_t)result > 32;
}

}