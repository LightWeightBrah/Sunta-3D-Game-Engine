#pragma once
#include <filesystem>

namespace Sunta
{

// The starting content of files made with "Create New ..." in the File Browser
// Each function returns false when the file couldn't be written
class AssetTemplates
{
public:
	static bool WriteScene(const std::filesystem::path& path);
	static bool WriteLuaScript(const std::filesystem::path& path);
	static bool WriteShader(const std::filesystem::path& path);
};

}
