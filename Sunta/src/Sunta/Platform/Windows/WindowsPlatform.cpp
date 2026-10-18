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

void WindowsPlatform::ShowErrorMessage(const std::string& title, const std::string& message)
{
	// The texts are in the ANSI code page, like everything from std::filesystem::path::string() on Windows
	MessageBoxA(nullptr, message.c_str(), title.c_str(), MB_OK | MB_ICONERROR | MB_SETFOREGROUND);
}

}