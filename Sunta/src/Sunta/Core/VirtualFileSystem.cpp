#include "SuntaPreCompiled.h"
#include "VirtualFileSystem.h"

namespace Sunta
{

std::unordered_map<std::string, std::filesystem::path>& VirtualFileSystem::GetMounts()
{
	static std::unordered_map<std::string, std::filesystem::path> mounts;
	return mounts;
}

void VirtualFileSystem::Init()
{
#if defined(SUNTA_DEVELOPMENT)
	std::filesystem::path rootDirectory = SUNTA_ROOT_DIR;
	Mount("@engine", rootDirectory / "Sunta" / "res");
	Mount("@game"  , rootDirectory / "Game"  / "res");
#else
	std::filesystem::path baseDirectory = std::filesystem::current_path();
	Mount("@engine", baseDirectory / "res" / "Sunta");
	Mount("@game"  , baseDirectory / "res" / "Game" );
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

}