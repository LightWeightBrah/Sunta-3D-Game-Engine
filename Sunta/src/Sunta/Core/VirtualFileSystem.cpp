#include "SuntaPreCompiled.h"
#include "VirtualFileSystem.h"

#if defined(SUNTA_PLATFORM_MAC)
	#include <mach-o/dyld.h>
#endif

namespace Sunta
{

std::unordered_map<std::string, std::filesystem::path>& VirtualFileSystem::GetMounts()
{
	static std::unordered_map<std::string, std::filesystem::path> mounts;
	return mounts;
}

void VirtualFileSystem::Init(AssetSource requestedSource, const std::filesystem::path& projectDirectory)
{
	GetMounts().clear();

	AssetSource assetSource = requestedSource;

#if defined(SUNTA_DEVELOPMENT)
	// Running from the IDE: the build folder has no "res" yet, so read from the source tree instead
	const bool hasPackagedResources = std::filesystem::exists(GetPackagedResourcesDirectory());
	if (requestedSource == AssetSource::PackagedNextToExecutable && !hasPackagedResources)
		assetSource = AssetSource::SourceTree;
#endif

	if (assetSource == AssetSource::SourceTree)
		MountSourceTree(projectDirectory);
	else
		MountPackagedResources();
}

void VirtualFileSystem::MountSourceTree(const std::filesystem::path& projectDirectory)
{
	const std::filesystem::path rootDirectory = SUNTA_ROOT_DIR;
	Mount("@engine", rootDirectory / "Sunta" / "res");
	Mount("@game"  , projectDirectory / "res");
}

void VirtualFileSystem::MountPackagedResources()
{
	const std::filesystem::path resourcesDirectory = GetPackagedResourcesDirectory();
	Mount("@engine", resourcesDirectory / "Sunta");
	Mount("@game"  , resourcesDirectory / "Game");
}

std::filesystem::path VirtualFileSystem::GetPackagedResourcesDirectory()
{
	return GetExecutableDirectory() / "res";
}

// Not current_path(): that is the folder the game was launched FROM (shortcut, terminal), not where it lies
std::filesystem::path VirtualFileSystem::GetExecutableDirectory()
{
#if defined(SUNTA_PLATFORM_WINDOWS)
	std::wstring executablePath(32768, L'\0');
	const DWORD length = GetModuleFileNameW(nullptr, executablePath.data(), static_cast<DWORD>(executablePath.size()));
	executablePath.resize(length);
	return std::filesystem::path(executablePath).parent_path();

#elif defined(SUNTA_PLATFORM_LINUX)
	std::error_code error;
	return std::filesystem::read_symlink("/proc/self/exe", error).parent_path();

#elif defined(SUNTA_PLATFORM_MAC)
	char executablePath[4096];
	uint32_t size = sizeof(executablePath);
	if (_NSGetExecutablePath(executablePath, &size) != 0)
		return std::filesystem::current_path();

	std::error_code error;
	return std::filesystem::canonical(executablePath, error).parent_path();

#else
	return std::filesystem::current_path();
#endif
}

void VirtualFileSystem::Mount(const std::string& virtualPrefix, const std::filesystem::path& physicalPath)
{
	GetMounts()[virtualPrefix] = std::filesystem::absolute(physicalPath);
}

std::string VirtualFileSystem::Resolve(const std::string& virtualPath)
{
	// go through all registered mount points (e.g. "@engine" => "C:/Dev/Sunta/res")
	for (const auto& [prefix, physicalRoot] : GetMounts())
	{
		// check if virtualPath starts with the prefix (e.g "@engine"), rfind starting at index 0
		if (virtualPath.rfind(prefix, 0) == 0)
		{
			// remove the prefix from the path (e.g "@engine/Textures/a.png" => "/Textures/a.png")
			std::string subPath = virtualPath.substr(prefix.length());

			// remove / or \ at the beginning if present (e.g "/Textures/a.png" => "Textures/a.png")
			if (!subPath.empty() && (subPath[0] == '/' || subPath[0] == '\\'))
				subPath = subPath.substr(1);

			// "C:/Dev/Sunta/res/Textures/a.png"
			return (physicalRoot / subPath).string();
		}
	}

	// fallback: return original path if no virtual prefix matched (e.g "Textures/a.png" or "C:/Textures/a.png")
	return virtualPath;
}

std::string VirtualFileSystem::ToVirtualPath(const std::string& physicalPath)
{
	const bool isAlreadyVirtual = !physicalPath.empty() && physicalPath[0] == '@';
	if (isAlreadyVirtual)
		return physicalPath;

	const std::filesystem::path normalizedPath = std::filesystem::absolute(physicalPath).lexically_normal();

	for (const auto& [prefix, physicalRoot] : GetMounts())
	{
		const std::filesystem::path relativePath = normalizedPath.lexically_relative(physicalRoot.lexically_normal());

		const bool isInsideMount = !relativePath.empty() && relativePath.begin()->string() != "..";
		if (isInsideMount)
			return prefix + "/" + relativePath.generic_string();
	}

	return physicalPath;
}

}
