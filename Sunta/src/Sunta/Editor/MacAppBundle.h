#pragma once
#include <filesystem>
#include <string>

namespace Sunta
{

// The macOS package of a game: <GameName>.app, a folder the Finder shows as one application with an icon
//   <GameName>.app/Contents/Info.plist
//   <GameName>.app/Contents/Resources/<GameName>.icns   icon made from the PNG of the project
//   <GameName>.app/Contents/MacOS/<GameName>            the game, its "res" folder lies next to it
// The code only writes files, so it compiles everywhere. GameBuilder uses it when the editor runs on macOS
class MacAppBundle
{
public:
	// Folder of the executable and of the "res" folder
	static std::filesystem::path GetExecutableDirectory(const std::filesystem::path& bundleDirectory);

	// Writes Info.plist. iconFileName is the file in Contents/Resources, empty for a bundle without an icon
	static bool WriteInfoPlist(const std::filesystem::path& bundleDirectory, const std::string& gameName, const std::string& iconFileName);

	// Makes <GameName>.icns in Contents/Resources from a PNG image. Returns the name of the file, empty on failure
	static std::string WriteIcon(const std::filesystem::path& bundleDirectory, const std::string& gameName, const std::filesystem::path& imagePath);
};

}
