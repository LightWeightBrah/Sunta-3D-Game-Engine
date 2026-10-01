#include "SuntaPreCompiled.h"
#include "Platform.h"

#include <nfd.hpp>

#include "Assert.h"

#if   defined(SUNTA_PLATFORM_WINDOWS)
	#include "Platform/Windows/WindowsPlatform.h"
#elif defined(SUNTA_PLATFORM_LINUX)
	#include "Platform/Linux/LinuxPlatform.h"
#elif defined(SUNTA_PLATFORM_MAC)
	#include "Platform/Mac/MacPlatform.h"
#endif    

namespace Sunta
{

void Platform::Init()
{
	NFD_Init();
	instance = Create();
}

void Platform::Shutdown()
{
	instance.reset();
	NFD_Quit();
}

std::string Platform::OpenFileDialog(const std::string& filterName, const std::vector<std::string>& extensions, const std::string& defaultPath)
{
	std::string formattedExtensions;
	for (unsigned int i = 0; i < extensions.size(); i++)
	{
		formattedExtensions += extensions[i];
		if (i + 1 < extensions.size())
			formattedExtensions += ",";
	}

	std::vector<nfdu8filteritem_t> filterItems;
	if (!formattedExtensions.empty())
		filterItems.push_back({ filterName.c_str(), formattedExtensions.c_str() });

	NFD::UniquePath outPath;
	nfdresult_t result = NFD::OpenDialog(outPath,
		filterItems.empty() ? nullptr : filterItems.data(),
		static_cast<nfdfiltersize_t>(filterItems.size()),
		defaultPath.empty() ? nullptr : defaultPath.c_str());

	if (result == NFD_OKAY)
	{
		std::filesystem::path absolutePath(outPath.get());
		std::filesystem::path currentPath = std::filesystem::current_path();

		try
		{
			return std::filesystem::relative(absolutePath, currentPath).generic_string();
		}
		catch (...)
		{
			return absolutePath.generic_string();
		}
		
	}

	return "";
}

std::string Platform::SaveFileDialog(const std::string& filterName, const std::vector<std::string>& extensions, const std::string& defaultPath)
{
	std::string formattedExtensions;
	for (unsigned int i = 0; i < extensions.size(); i++)
	{
		formattedExtensions += extensions[i];
		if (i + 1 < extensions.size())
			formattedExtensions += ",";
	}

	std::vector<nfdu8filteritem_t> filterItems;
	if (!formattedExtensions.empty())
		filterItems.push_back({ filterName.c_str(), formattedExtensions.c_str() });

	NFD::UniquePath outPath;
	nfdresult_t result = NFD::SaveDialog(outPath,
		filterItems.empty() ? nullptr : filterItems.data(),
		static_cast<nfdfiltersize_t>(filterItems.size()),
		defaultPath.empty() ? nullptr : defaultPath.c_str());

	if (result == NFD_OKAY)
	{
		std::filesystem::path absolutePath(outPath.get());

		if (!extensions.empty() && !absolutePath.has_extension())
		{
			std::string extensionToAdd = extensions[0];
			if (extensionToAdd[0] != '.')
				extensionToAdd = "." + extensionToAdd;
			
			absolutePath += extensionToAdd;

		}

		std::filesystem::path currentPath = std::filesystem::current_path();

		try
		{
			return std::filesystem::relative(absolutePath, currentPath).generic_string();
		}
		catch (...)
		{
			return absolutePath.generic_string();
		}

	}

	return "";
}

std::unique_ptr<Platform> Platform::Create()
{
#if   defined(SUNTA_PLATFORM_WINDOWS)
	return std::make_unique<WindowsPlatform>();

#elif defined(SUNTA_PLATFORM_LINUX)
	return std::make_unique<LinuxPlatform>();

#elif defined(SUNTA_PLATFORM_MAC)
	return std::make_unique<MacPlatform>();

#else
	SUNTA_ASSERT(false, "Unsupported platform! Sunta Engine does not support this platform yet.");
	return nullptr;

#endif
}

}