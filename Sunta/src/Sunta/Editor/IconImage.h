#pragma once
#include <filesystem>
#include <vector>

namespace Sunta
{

// A square image, 4 bytes (R, G, B, A) per pixel, rows from the top
struct SquareImage
{
	int                        size = 0;
	std::vector<unsigned char> pixels;
};

// Image operations for building the icons of games from one PNG: reading, scaling and writing PNG
namespace IconImage
{
	// Reads the whole file. Reading it first (instead of giving a path to stb) makes paths with non-English characters work on Windows
	bool ReadFileBytes(const std::filesystem::path& path, std::vector<unsigned char>& outBytes);

	// Loads the image and puts it in the middle of a transparent square, so a non-square image isn't stretched
	bool Load(const std::filesystem::path& imagePath, SquareImage& outImage);

	// Average of the source pixels covered by every target pixel (box filter), colors weighted by alpha
	SquareImage Resize(const SquareImage& source, int targetSize);

	// Content of a .png file. The pixel data is stored without compression (valid PNG, no zlib needed), icons are small
	std::vector<unsigned char> EncodePng(const SquareImage& image);
}

}
