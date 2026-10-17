#pragma once
#include <filesystem>

namespace Sunta
{

// Puts the icon of a game into its executable, so the game has its own icon in the file explorer
// (the icon of the window itself is set by the game at runtime from game.config)
//   Windows: rewrites the icon resources of the .exe from a PNG image, no compiler needed
//   Other systems: not supported here, returns false (Linux uses a .desktop file, see GameBuilder)
class ExecutableIcon
{
public:
	// Size of the image in pixels. Returns false when the file can't be read as an image
	static bool TryReadImageSize(const std::filesystem::path& imagePath, int& outWidth, int& outHeight);

	static bool Apply(const std::filesystem::path& executablePath, const std::filesystem::path& imagePath);
};

}
