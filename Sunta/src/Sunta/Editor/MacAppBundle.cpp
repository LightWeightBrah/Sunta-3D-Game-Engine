#include "Core/SuntaPreCompiled.h"
#include "MacAppBundle.h"

#include "IconImage.h"
#include "Core/Log.h"

namespace Sunta
{

namespace
{
	namespace fs = std::filesystem;

	// Entries of an .icns file: the type tells the size, the data is a PNG (the Finder reads these since macOS 10.7)
	struct IcnsEntry
	{
		const char* type;
		int         size;
	};

	// Sizes bigger than the source image are skipped (no blurry upscaling)
	const std::vector<IcnsEntry> IcnsEntries =
	{
		{ "icp4",   16 },
		{ "icp5",   32 },
		{ "icp6",   64 },
		{ "ic07",  128 },
		{ "ic08",  256 },
		{ "ic09",  512 },
		{ "ic10", 1024 },
	};

	constexpr int         IcnsHeaderBytes = 8;   // 4 characters of type + 4 bytes of length
	constexpr const char* IcnsFileMagic   = "icns";
	constexpr const char* ContentsFolder  = "Contents";

	void AppendBigEndian32(std::vector<unsigned char>& bytes, uint32_t value)
	{
		bytes.push_back(static_cast<unsigned char>(value >> 24));
		bytes.push_back(static_cast<unsigned char>(value >> 16));
		bytes.push_back(static_cast<unsigned char>(value >> 8));
		bytes.push_back(static_cast<unsigned char>(value));
	}

	void AppendIcnsEntry(std::vector<unsigned char>& icns, const char* type, const std::vector<unsigned char>& png)
	{
		icns.insert(icns.end(), type, type + 4);
		AppendBigEndian32(icns, static_cast<uint32_t>(IcnsHeaderBytes + png.size()));
		icns.insert(icns.end(), png.begin(), png.end());
	}

	std::vector<unsigned char> CreateIcns(const SquareImage& source)
	{
		std::vector<unsigned char> icns(IcnsFileMagic, IcnsFileMagic + 4);
		AppendBigEndian32(icns, 0);   // the total length is known at the end

		bool hasEntry = false;
		for (const IcnsEntry& entry : IcnsEntries)
		{
			if (entry.size > source.size)
				continue;

			AppendIcnsEntry(icns, entry.type, IconImage::EncodePng(IconImage::Resize(source, entry.size)));
			hasEntry = true;
		}

		// A tiny source image still gets an icon (the smallest size)
		if (!hasEntry)
			AppendIcnsEntry(icns, IcnsEntries.front().type, IconImage::EncodePng(IconImage::Resize(source, IcnsEntries.front().size)));

		const uint32_t totalLength = static_cast<uint32_t>(icns.size());
		icns[4] = static_cast<unsigned char>(totalLength >> 24);
		icns[5] = static_cast<unsigned char>(totalLength >> 16);
		icns[6] = static_cast<unsigned char>(totalLength >> 8);
		icns[7] = static_cast<unsigned char>(totalLength);

		return icns;
	}

	std::string EscapeXml(const std::string& text)
	{
		std::string escaped;

		for (char character : text)
		{
			switch (character)
			{
				case '&': escaped += "&amp;";  break;
				case '<': escaped += "&lt;";   break;
				case '>': escaped += "&gt;";   break;
				case '"': escaped += "&quot;"; break;
				default:  escaped += character;
			}
		}

		return escaped;
	}

	// Bundle identifiers allow only letters, digits, '.' and '-'
	std::string MakeBundleIdentifier(const std::string& gameName)
	{
		std::string identifier = "com.sunta.";

		for (char character : gameName)
		{
			const bool isAllowed = std::isalnum(static_cast<unsigned char>(character)) != 0;
			identifier += isAllowed ? character : '-';
		}

		return identifier;
	}
}

fs::path MacAppBundle::GetExecutableDirectory(const fs::path& bundleDirectory)
{
	return bundleDirectory / ContentsFolder / "MacOS";
}

bool MacAppBundle::WriteInfoPlist(const fs::path& bundleDirectory, const std::string& gameName, const std::string& iconFileName)
{
	std::error_code error;
	fs::create_directories(bundleDirectory / ContentsFolder, error);

	std::ofstream plist(bundleDirectory / ContentsFolder / "Info.plist");
	if (!plist.is_open())
		return false;

	const std::string escapedName = EscapeXml(gameName);

	plist << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
		  << "<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">\n"
		  << "<plist version=\"1.0\">\n"
		  << "<dict>\n"
		  << "\t<key>CFBundleName</key><string>" << escapedName << "</string>\n"
		  << "\t<key>CFBundleDisplayName</key><string>" << escapedName << "</string>\n"
		  << "\t<key>CFBundleExecutable</key><string>" << escapedName << "</string>\n"
		  << "\t<key>CFBundleIdentifier</key><string>" << EscapeXml(MakeBundleIdentifier(gameName)) << "</string>\n"
		  << "\t<key>CFBundlePackageType</key><string>APPL</string>\n"
		  << "\t<key>CFBundleInfoDictionaryVersion</key><string>6.0</string>\n"
		  << "\t<key>CFBundleVersion</key><string>1.0</string>\n"
		  << "\t<key>CFBundleShortVersionString</key><string>1.0</string>\n"
		  << "\t<key>NSHighResolutionCapable</key><true/>\n";

	if (!iconFileName.empty())
		plist << "\t<key>CFBundleIconFile</key><string>" << EscapeXml(iconFileName) << "</string>\n";

	plist << "</dict>\n"
		  << "</plist>\n";

	return plist.good();
}

std::string MacAppBundle::WriteIcon(const fs::path& bundleDirectory, const std::string& gameName, const fs::path& imagePath)
{
	SquareImage sourceImage;
	if (!IconImage::Load(imagePath, sourceImage))
	{
		SUNTA_ENGINE_LOG_ERROR("MacAppBundle: Couldn't read the image '{0}'", imagePath.string());
		return std::string();
	}

	const std::string iconFileName    = gameName + ".icns";
	const fs::path    resourcesFolder = bundleDirectory / ContentsFolder / "Resources";

	std::error_code error;
	fs::create_directories(resourcesFolder, error);

	const std::vector<unsigned char> icns = CreateIcns(sourceImage);

	std::ofstream iconFile(resourcesFolder / iconFileName, std::ios::binary);
	if (!iconFile.is_open())
		return std::string();

	iconFile.write(reinterpret_cast<const char*>(icns.data()), static_cast<std::streamsize>(icns.size()));
	return iconFile.good() ? iconFileName : std::string();
}

}
