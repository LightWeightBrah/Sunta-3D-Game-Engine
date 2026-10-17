#pragma once
#include <string>
#include <filesystem>
#include <unordered_map>

namespace Sunta
{

enum class AssetSource
{
	SourceTree,               // reads assets straight from Sunta/res and <project>/res (the editor, development)
	PackagedNextToExecutable  // reads assets from the "res" folder lying next to the executable (shipped game)
};

class VirtualFileSystem
{
public:
	// projectDirectory: folder of the game project (contains "res"), mounted as "@game" in SourceTree mode
	// Ignored by PackagedNextToExecutable, which has its own "res/Game"
	static void Init(AssetSource assetSource, const std::filesystem::path& projectDirectory);

	static void Mount(const std::string& virtualPrefix, const std::filesystem::path& physicalPath);

	// "@game/Scripts/a.lua" => "C:/Dev/Sunta/Game/res/Scripts/a.lua"
	static std::string Resolve(const std::string& virtualPath);

	// "C:/Dev/Sunta/Game/res/Scripts/a.lua" => "@game/Scripts/a.lua"
	// Returns the path unchanged if it isn't inside any mount
	static std::string ToVirtualPath(const std::string& physicalPath);

	static std::filesystem::path GetExecutableDirectory();

private:
	static std::unordered_map<std::string, std::filesystem::path>& GetMounts();

	static void MountSourceTree(const std::filesystem::path& projectDirectory);
	static void MountPackagedResources();
	static std::filesystem::path GetPackagedResourcesDirectory();
};

}
