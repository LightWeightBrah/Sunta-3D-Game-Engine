#include "Core/SuntaPreCompiled.h"
#include "IconImage.h"

#include <stb/stb_image.h>

namespace Sunta
{

namespace
{
	namespace fs = std::filesystem;

	constexpr int    PixelBytes            = 4;
	constexpr size_t MaxStoredBlockBytes   = 65535;
	constexpr size_t AdlerModulus          = 65521;
	constexpr unsigned char PngColorTypeRgba = 6;
	constexpr unsigned char PngBitDepth      = 8;
	constexpr unsigned char PngFilterNone    = 0;

	const unsigned char PngSignature[] = { 0x89, 'P', 'N', 'G', 0x0D, 0x0A, 0x1A, 0x0A };

	unsigned char ToByte(float value)
	{
		return static_cast<unsigned char>(std::clamp(std::lround(value), 0L, 255L));
	}

	void AppendBigEndian32(std::vector<unsigned char>& bytes, uint32_t value)
	{
		bytes.push_back(static_cast<unsigned char>(value >> 24));
		bytes.push_back(static_cast<unsigned char>(value >> 16));
		bytes.push_back(static_cast<unsigned char>(value >> 8));
		bytes.push_back(static_cast<unsigned char>(value));
	}

	uint32_t UpdateCrc32(uint32_t crc, const unsigned char* data, size_t length)
	{
		static const std::vector<uint32_t> table = []
		{
			std::vector<uint32_t> values(256);
			for (uint32_t index = 0; index < 256; ++index)
			{
				uint32_t value = index;
				for (int bit = 0; bit < 8; ++bit)
					value = (value & 1) ? (0xEDB88320u ^ (value >> 1)) : (value >> 1);
				values[index] = value;
			}
			return values;
		}();

		for (size_t index = 0; index < length; ++index)
			crc = table[(crc ^ data[index]) & 0xFF] ^ (crc >> 8);

		return crc;
	}

	uint32_t CalculateAdler32(const std::vector<unsigned char>& data)
	{
		size_t low = 1, high = 0;

		for (unsigned char byte : data)
		{
			low  = (low + byte) % AdlerModulus;
			high = (high + low) % AdlerModulus;
		}

		return static_cast<uint32_t>((high << 16) | low);
	}

	// PNG chunk: length, type, data, CRC of type and data
	void AppendChunk(std::vector<unsigned char>& png, const char (&type)[5], const std::vector<unsigned char>& data)
	{
		AppendBigEndian32(png, static_cast<uint32_t>(data.size()));

		const size_t typeStart = png.size();
		png.insert(png.end(), type, type + 4);
		png.insert(png.end(), data.begin(), data.end());

		const uint32_t crc = UpdateCrc32(0xFFFFFFFFu, png.data() + typeStart, png.size() - typeStart) ^ 0xFFFFFFFFu;
		AppendBigEndian32(png, crc);
	}

	// Every row starts with a filter byte (none), then the R, G, B, A of its pixels
	std::vector<unsigned char> CreateFilteredRows(const SquareImage& image)
	{
		const size_t rowBytes = static_cast<size_t>(image.size) * PixelBytes;

		std::vector<unsigned char> rows;
		rows.reserve((rowBytes + 1) * image.size);

		for (int row = 0; row < image.size; ++row)
		{
			rows.push_back(PngFilterNone);

			const unsigned char* source = &image.pixels[row * rowBytes];
			rows.insert(rows.end(), source, source + rowBytes);
		}

		return rows;
	}

	// zlib stream made of "stored" deflate blocks (no compression)
	std::vector<unsigned char> CreateZlibStream(const std::vector<unsigned char>& data)
	{
		std::vector<unsigned char> stream = { 0x78, 0x01 };

		size_t offset = 0;
		do
		{
			const size_t blockSize = std::min(MaxStoredBlockBytes, data.size() - offset);
			const bool   isLast    = (offset + blockSize == data.size());

			stream.push_back(isLast ? 1 : 0);
			stream.push_back(static_cast<unsigned char>(blockSize & 0xFF));
			stream.push_back(static_cast<unsigned char>(blockSize >> 8));
			stream.push_back(static_cast<unsigned char>(~blockSize & 0xFF));
			stream.push_back(static_cast<unsigned char>((~blockSize >> 8) & 0xFF));

			stream.insert(stream.end(), data.begin() + offset, data.begin() + offset + blockSize);
			offset += blockSize;
		}
		while (offset < data.size());

		AppendBigEndian32(stream, CalculateAdler32(data));
		return stream;
	}
}

bool IconImage::ReadFileBytes(const fs::path& path, std::vector<unsigned char>& outBytes)
{
	std::ifstream file(path, std::ios::binary);
	if (!file.is_open())
		return false;

	outBytes.assign(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
	return !outBytes.empty();
}

bool IconImage::Load(const fs::path& imagePath, SquareImage& outImage)
{
	std::vector<unsigned char> bytes;
	if (!ReadFileBytes(imagePath, bytes))
		return false;

	int width = 0, height = 0, channels = 0;
	stbi_set_flip_vertically_on_load(false);
	unsigned char* data = stbi_load_from_memory(bytes.data(), static_cast<int>(bytes.size()), &width, &height, &channels, PixelBytes);
	if (!data)
		return false;

	const int size    = std::max(width, height);
	const int offsetX = (size - width)  / 2;
	const int offsetY = (size - height) / 2;

	outImage.size = size;
	outImage.pixels.assign(static_cast<size_t>(size) * size * PixelBytes, 0);

	for (int row = 0; row < height; ++row)
	{
		unsigned char*       target = &outImage.pixels[(static_cast<size_t>(row + offsetY) * size + offsetX) * PixelBytes];
		const unsigned char* source = data + static_cast<size_t>(row) * width * PixelBytes;
		std::memcpy(target, source, static_cast<size_t>(width) * PixelBytes);
	}

	stbi_image_free(data);
	return true;
}

SquareImage IconImage::Resize(const SquareImage& source, int targetSize)
{
	SquareImage target;
	target.size = targetSize;
	target.pixels.resize(static_cast<size_t>(targetSize) * targetSize * PixelBytes);

	const float scale = static_cast<float>(source.size) / static_cast<float>(targetSize);

	for (int targetY = 0; targetY < targetSize; ++targetY)
	{
		const int firstY = std::min(static_cast<int>(targetY * scale), source.size - 1);
		const int lastY  = std::min(std::max(firstY + 1, static_cast<int>(std::ceil((targetY + 1) * scale))), source.size);

		for (int targetX = 0; targetX < targetSize; ++targetX)
		{
			const int firstX = std::min(static_cast<int>(targetX * scale), source.size - 1);
			const int lastX  = std::min(std::max(firstX + 1, static_cast<int>(std::ceil((targetX + 1) * scale))), source.size);

			float red = 0.0f, green = 0.0f, blue = 0.0f, alphaSum = 0.0f;
			int   pixelCount = 0;

			for (int sourceY = firstY; sourceY < lastY; ++sourceY)
			{
				for (int sourceX = firstX; sourceX < lastX; ++sourceX)
				{
					const unsigned char* pixel = &source.pixels[(static_cast<size_t>(sourceY) * source.size + sourceX) * PixelBytes];
					const float alpha = pixel[3] / 255.0f;

					red      += pixel[0] * alpha;
					green    += pixel[1] * alpha;
					blue     += pixel[2] * alpha;
					alphaSum += alpha;
					++pixelCount;
				}
			}

			unsigned char* targetPixel = &target.pixels[(static_cast<size_t>(targetY) * targetSize + targetX) * PixelBytes];
			const bool isVisible = alphaSum > 0.0f;

			targetPixel[0] = isVisible ? ToByte(red   / alphaSum) : 0;
			targetPixel[1] = isVisible ? ToByte(green / alphaSum) : 0;
			targetPixel[2] = isVisible ? ToByte(blue  / alphaSum) : 0;
			targetPixel[3] = ToByte(alphaSum / pixelCount * 255.0f);
		}
	}

	return target;
}

std::vector<unsigned char> IconImage::EncodePng(const SquareImage& image)
{
	std::vector<unsigned char> header;
	AppendBigEndian32(header, static_cast<uint32_t>(image.size));   // width
	AppendBigEndian32(header, static_cast<uint32_t>(image.size));   // height
	header.push_back(PngBitDepth);
	header.push_back(PngColorTypeRgba);
	header.push_back(0);   // compression method
	header.push_back(0);   // filter method
	header.push_back(0);   // no interlacing

	std::vector<unsigned char> png(std::begin(PngSignature), std::end(PngSignature));
	AppendChunk(png, "IHDR", header);
	AppendChunk(png, "IDAT", CreateZlibStream(CreateFilteredRows(image)));
	AppendChunk(png, "IEND", {});

	return png;
}

}
