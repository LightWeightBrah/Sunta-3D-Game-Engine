#include "Core/SuntaPreCompiled.h"

#include "WindowsPlatform.h"

namespace Sunta
{

bool WindowsPlatform::OpenInExplorer(const std::string& path)
{
	std::filesystem::path absolutePath = std::filesystem::absolute(path);

	std::string args = "/select,\"" + absolutePath.string() + "\"";
	HINSTANCE result = ShellExecuteA(NULL, "open", "explorer.exe", args.c_str(), NULL, SW_SHOWNORMAL);
	return (intptr_t)result > 32;
}

bool WindowsPlatform::OpenFileExternally(const std::string& path)
{
	std::filesystem::path absolutePath = std::filesystem::absolute(path);

	HINSTANCE result = ShellExecuteA(NULL, "open", absolutePath.string().c_str(), NULL, NULL, SW_SHOWNORMAL);
	return (intptr_t)result > 32;
}

}