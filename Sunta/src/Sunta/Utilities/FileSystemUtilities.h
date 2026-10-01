#pragma once

#include <string>
#include <unordered_set>
#include <algorithm>

namespace Sunta
{

inline bool IsTextureExtension(std::string extension)
{
	std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
	static const std::unordered_set<std::string> possibleExtensions =
	{
		".png", ".jpg", ".jpeg", ".tga", ".bmp", ".psd", ".hdr"
	};

	return possibleExtensions.find(extension) != possibleExtensions.end();
}

inline bool IsModelExtension(std::string extension)
{
	std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
	static const std::unordered_set<std::string> possibleExtensions =
	{
		".obj", ".fbx", ".gltf", ".glb", ".dae"
	};

	return possibleExtensions.find(extension) != possibleExtensions.end();
}

inline bool IsAudioExtension(std::string extension)
{
	std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
	static const std::unordered_set<std::string> possibleExtensions =
	{
		".wav", ".mp3", ".ogg"
	};

	return possibleExtensions.find(extension) != possibleExtensions.end();
}

inline bool IsMaterialExtension(std::string extension)
{
	std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
	return extension == ".material";
}

inline bool IsShaderExtension(std::string extension)
{
	std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
	return extension == ".shader";
}

inline bool IsSceneExtension(std::string extension)
{
	std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
	return extension == ".scene";
}

}