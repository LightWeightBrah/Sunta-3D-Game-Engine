#include "Core/SuntaPreCompiled.h"
#include "ProcessLauncher.h"

#include "Core/Log.h"
#include "Core/VirtualFileSystem.h"

#if !defined(SUNTA_PLATFORM_WINDOWS)
	#include <sys/types.h>
	#include <sys/wait.h>
	#include <unistd.h>
#endif

namespace Sunta
{

namespace
{

#if defined(SUNTA_PLATFORM_WINDOWS)
std::string WideToUtf8(const std::wstring& wide)
{
	if (wide.empty())
		return std::string();

	const int size = WideCharToMultiByte(CP_UTF8, 0, wide.data(), static_cast<int>(wide.size()), nullptr, 0, nullptr, nullptr);

	std::string utf8(size, '\0');
	WideCharToMultiByte(CP_UTF8, 0, wide.data(), static_cast<int>(wide.size()), utf8.data(), size, nullptr, nullptr);
	return utf8;
}
#endif

}

std::vector<std::string> ProcessLauncher::GetCommandLineArguments(int argc, char** argv)
{
	std::vector<std::string> arguments;

#if defined(SUNTA_PLATFORM_WINDOWS)
	int wideArgumentCount = 0;
	LPWSTR* wideArguments = CommandLineToArgvW(GetCommandLineW(), &wideArgumentCount);

	if (wideArguments)
	{
		for (int index = 0; index < wideArgumentCount; ++index)
			arguments.push_back(WideToUtf8(wideArguments[index]));

		LocalFree(wideArguments);
		return arguments;
	}
#endif

	for (int index = 0; index < argc; ++index)
		arguments.push_back(argv[index]);

	return arguments;
}

bool ProcessLauncher::StartEditorWithProject(const std::filesystem::path& projectDirectory)
{
	// generic_u8string: '/' instead of '\' also keeps the closing quote on Windows safe (no "C:\Folder\")
	const std::string projectArgument = projectDirectory.lexically_normal().generic_u8string();
	return StartDetached(VirtualFileSystem::GetExecutablePath(), { projectArgument });
}

bool ProcessLauncher::StartEditorLauncher()
{
	return StartDetached(VirtualFileSystem::GetExecutablePath(), { LauncherArgument });
}

#if defined(SUNTA_PLATFORM_WINDOWS)

bool ProcessLauncher::StartDetached(const std::filesystem::path& executablePath, const std::vector<std::string>& arguments)
{
	std::wstring commandLine = L"\"" + executablePath.wstring() + L"\"";
	for (const std::string& argument : arguments)
		commandLine += L" \"" + std::filesystem::u8path(argument).wstring() + L"\"";

	STARTUPINFOW startupInfo = {};
	startupInfo.cb = sizeof(startupInfo);
	PROCESS_INFORMATION processInfo = {};

	// CreateProcessW may change the command line buffer, so it has to be writable
	const BOOL isStarted = CreateProcessW(executablePath.c_str(), commandLine.data(), nullptr, nullptr,
										  FALSE, 0, nullptr, nullptr, &startupInfo, &processInfo);

	if (!isStarted)
	{
		SUNTA_ENGINE_LOG_ERROR("ProcessLauncher: Couldn't start '{0}' (error {1})", executablePath.string(), GetLastError());
		return false;
	}

	CloseHandle(processInfo.hThread);
	CloseHandle(processInfo.hProcess);
	return true;
}

#else

bool ProcessLauncher::StartDetached(const std::filesystem::path& executablePath, const std::vector<std::string>& arguments)
{
	const std::string executable = executablePath.string();

	std::vector<char*> argumentPointers;
	argumentPointers.push_back(const_cast<char*>(executable.c_str()));
	for (const std::string& argument : arguments)
		argumentPointers.push_back(const_cast<char*>(argument.c_str()));
	argumentPointers.push_back(nullptr);

	// Double fork: the new program is adopted by init, so it neither stays a zombie nor dies with this editor
	const pid_t child = fork();
	if (child < 0)
	{
		SUNTA_ENGINE_LOG_ERROR("ProcessLauncher: fork failed");
		return false;
	}

	if (child == 0)
	{
		if (fork() == 0)
		{
			setsid();
			execv(executable.c_str(), argumentPointers.data());
			_exit(127);
		}

		_exit(0);
	}

	int status = 0;
	waitpid(child, &status, 0);
	return true;
}

#endif

}
