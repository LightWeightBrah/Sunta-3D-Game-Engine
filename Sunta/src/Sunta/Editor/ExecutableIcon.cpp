#include "Core/SuntaPreCompiled.h"
#include "ExecutableIcon.h"

#include <stb/stb_image.h>

#include "IconImage.h"
#include "Core/Log.h"

namespace Sunta
{

namespace
{
	namespace fs = std::filesystem;
}

bool ExecutableIcon::TryReadImageSize(const fs::path& imagePath, int& outWidth, int& outHeight)
{
	std::vector<unsigned char> bytes;
	if (!IconImage::ReadFileBytes(imagePath, bytes))
		return false;

	int channels = 0;
	return stbi_info_from_memory(bytes.data(), static_cast<int>(bytes.size()), &outWidth, &outHeight, &channels) != 0;
}

#if defined(SUNTA_PLATFORM_WINDOWS)

namespace
{
	// Sizes Windows shows in the explorer, taskbar, alt-tab... Bigger than the source image are skipped (no blurry upscaling)
	const std::vector<int> IconSizes = { 16, 24, 32, 48, 64, 128, 256 };

	constexpr WORD  IconResourceType  = 1;       // value of the "type" field in icon directories
	constexpr WORD  DefaultGroupId    = 1;
	constexpr int   MaxIconDirectoryEntrySize = 256; // a size of 256 is stored as 0 in the directory

#pragma pack(push, 2)
	struct GroupIconHeader
	{
		WORD reserved;
		WORD type;
		WORD count;
	};

	struct GroupIconEntry
	{
		BYTE  width;
		BYTE  height;
		BYTE  colorCount;
		BYTE  reserved;
		WORD  planes;
		WORD  bitCount;
		DWORD bytesInResource;
		WORD  id;
	};
#pragma pack(pop)

	// RT_ICON and RT_GROUP_ICON expand to narrow strings when the project is not built with UNICODE
	const LPCWSTR kIconResourceType      = MAKEINTRESOURCEW(3);    // RT_ICON
	const LPCWSTR kIconGroupResourceType = MAKEINTRESOURCEW(14);   // RT_GROUP_ICON

	// Resource name can be a number or a text, the icon of Sunta executables is named by text ("IDI_ICON1")
	struct ResourceName
	{
		std::wstring text;
		WORD         id       = 0;
		bool         isNumber = false;

		LPCWSTR Get() const { return isNumber ? MAKEINTRESOURCEW(id) : text.c_str(); }
	};

	struct ResourceEntry
	{
		ResourceName name;
		WORD         language = 0;
	};

	struct EnumerationContext
	{
		std::vector<ResourceEntry> entries;
		ResourceName               currentName;
	};

	BOOL CALLBACK CollectLanguage(HMODULE, LPCWSTR, LPCWSTR, WORD language, LONG_PTR parameter)
	{
		auto* context = reinterpret_cast<EnumerationContext*>(parameter);
		context->entries.push_back({ context->currentName, language });
		return TRUE;
	}

	BOOL CALLBACK CollectName(HMODULE module, LPCWSTR type, LPWSTR name, LONG_PTR parameter)
	{
		auto* context = reinterpret_cast<EnumerationContext*>(parameter);

		context->currentName = ResourceName();
		if (IS_INTRESOURCE(name))
		{
			context->currentName.isNumber = true;
			context->currentName.id       = static_cast<WORD>(reinterpret_cast<ULONG_PTR>(name));
		}
		else
		{
			context->currentName.text = name;
		}

		EnumResourceLanguagesW(module, type, name, CollectLanguage, parameter);
		return TRUE;
	}

	std::vector<ResourceEntry> ReadResources(const fs::path& executablePath, LPCWSTR type)
	{
		EnumerationContext context;

		HMODULE module = LoadLibraryExW(executablePath.c_str(), nullptr, LOAD_LIBRARY_AS_DATAFILE);
		if (!module)
			return context.entries;

		EnumResourceNamesW(module, type, CollectName, reinterpret_cast<LONG_PTR>(&context));

		// Has to be released before the file is updated
		FreeLibrary(module);
		return context.entries;
	}

	// The content of one RT_ICON resource: a bitmap with a header, rows from the BOTTOM, pixels as B, G, R, A,
	// followed by a 1-bit mask (all zeros, the alpha channel does the transparency)
	std::vector<unsigned char> EncodeBitmap(const SquareImage& image)
	{
		const int    size          = image.size;
		const size_t pixelBytes    = static_cast<size_t>(size) * size * 4;
		const size_t maskRowBytes  = static_cast<size_t>((size + 31) / 32) * 4;
		const size_t maskBytes     = maskRowBytes * size;

		BITMAPINFOHEADER header = {};
		header.biSize        = sizeof(BITMAPINFOHEADER);
		header.biWidth       = size;
		header.biHeight      = size * 2;       // the image and the mask together
		header.biPlanes      = 1;
		header.biBitCount    = 32;
		header.biCompression = BI_RGB;
		header.biSizeImage   = static_cast<DWORD>(pixelBytes + maskBytes);

		std::vector<unsigned char> data(sizeof(header) + pixelBytes + maskBytes, 0);
		std::memcpy(data.data(), &header, sizeof(header));

		unsigned char* target = data.data() + sizeof(header);
		for (int row = size - 1; row >= 0; --row)
		{
			const unsigned char* sourceRow = &image.pixels[static_cast<size_t>(row) * size * 4];

			for (int column = 0; column < size; ++column)
			{
				const unsigned char* pixel = sourceRow + column * 4;
				target[0] = pixel[2];
				target[1] = pixel[1];
				target[2] = pixel[0];
				target[3] = pixel[3];
				target += 4;
			}
		}

		return data;
	}

	std::vector<SquareImage> CreateIconImages(const SquareImage& source)
	{
		std::vector<SquareImage> images;

		for (int size : IconSizes)
		{
			if (size <= source.size)
				images.push_back(IconImage::Resize(source, size));
		}

		// A tiny source image still gets an icon (the smallest size)
		if (images.empty())
			images.push_back(IconImage::Resize(source, IconSizes.front()));

		return images;
	}

	std::vector<unsigned char> BuildGroupIconDirectory(const std::vector<SquareImage>& images, const std::vector<std::vector<unsigned char>>& bitmaps)
	{
		std::vector<unsigned char> directory(sizeof(GroupIconHeader) + sizeof(GroupIconEntry) * images.size());

		GroupIconHeader header = { 0, IconResourceType, static_cast<WORD>(images.size()) };
		std::memcpy(directory.data(), &header, sizeof(header));

		for (size_t index = 0; index < images.size(); ++index)
		{
			const bool isMaxSize = images[index].size >= MaxIconDirectoryEntrySize;

			GroupIconEntry entry = {};
			entry.width           = isMaxSize ? 0 : static_cast<BYTE>(images[index].size);
			entry.height          = entry.width;
			entry.planes          = 1;
			entry.bitCount        = 32;
			entry.bytesInResource = static_cast<DWORD>(bitmaps[index].size());
			entry.id              = static_cast<WORD>(index + 1);   // id of the RT_ICON resource

			std::memcpy(directory.data() + sizeof(header) + sizeof(entry) * index, &entry, sizeof(entry));
		}

		return directory;
	}
}

bool ExecutableIcon::Apply(const fs::path& executablePath, const fs::path& imagePath)
{
	SquareImage sourceImage;
	if (!IconImage::Load(imagePath, sourceImage))
	{
		SUNTA_ENGINE_LOG_ERROR("ExecutableIcon: Couldn't read the image '{0}'", imagePath.string());
		return false;
	}

	const std::vector<SquareImage> images = CreateIconImages(sourceImage);

	std::vector<std::vector<unsigned char>> bitmaps;
	for (const SquareImage& image : images)
		bitmaps.push_back(EncodeBitmap(image));

	const std::vector<unsigned char> groupDirectory = BuildGroupIconDirectory(images, bitmaps);

	// The old icon goes away completely. The new one keeps the name and language of the old one,
	// otherwise the explorer could still find the old icon first
	const std::vector<ResourceEntry> oldGroups = ReadResources(executablePath, kIconGroupResourceType);
	const std::vector<ResourceEntry> oldIcons  = ReadResources(executablePath, kIconResourceType);

	ResourceName groupName;
	groupName.isNumber = true;
	groupName.id       = DefaultGroupId;
	WORD language      = 0;

	if (!oldGroups.empty())
	{
		groupName = oldGroups.front().name;
		language  = oldGroups.front().language;
	}

	HANDLE update = BeginUpdateResourceW(executablePath.c_str(), FALSE);
	if (!update)
	{
		SUNTA_ENGINE_LOG_ERROR("ExecutableIcon: Couldn't open '{0}' for changes (error {1})", executablePath.string(), GetLastError());
		return false;
	}

	bool isUpdated = true;

	for (const ResourceEntry& oldGroup : oldGroups)
		isUpdated = isUpdated && UpdateResourceW(update, kIconGroupResourceType, oldGroup.name.Get(), oldGroup.language, nullptr, 0);

	for (const ResourceEntry& oldIcon : oldIcons)
		isUpdated = isUpdated && UpdateResourceW(update, kIconResourceType, oldIcon.name.Get(), oldIcon.language, nullptr, 0);

	for (size_t index = 0; index < bitmaps.size(); ++index)
	{
		isUpdated = isUpdated && UpdateResourceW(update, kIconResourceType, MAKEINTRESOURCEW(index + 1), language,
												 const_cast<unsigned char*>(bitmaps[index].data()), static_cast<DWORD>(bitmaps[index].size()));
	}

	isUpdated = isUpdated && UpdateResourceW(update, kIconGroupResourceType, groupName.Get(), language,
											 const_cast<unsigned char*>(groupDirectory.data()), static_cast<DWORD>(groupDirectory.size()));

	// When something failed, the changes are discarded and the executable stays as it was
	const BOOL isSaved = EndUpdateResourceW(update, isUpdated ? FALSE : TRUE);

	if (!isUpdated || !isSaved)
	{
		SUNTA_ENGINE_LOG_ERROR("ExecutableIcon: Changing the icon of '{0}' failed", executablePath.string());
		return false;
	}

	SUNTA_ENGINE_LOG_INFO("ExecutableIcon: Icon of '{0}' set from '{1}'", executablePath.filename().string(), imagePath.filename().string());
	return true;
}

#else

bool ExecutableIcon::Apply(const fs::path& executablePath, const fs::path& imagePath)
{
	(void)executablePath;
	(void)imagePath;

	SUNTA_ENGINE_LOG_WARNING("ExecutableIcon: Changing the icon of an executable is supported only on Windows");
	return false;
}

#endif

}
