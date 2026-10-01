#include "Core/SuntaPreCompiled.h"
#include "TextureNamingConventionsUtilities.h"

#include <fstream>
#include <algorithm>
#include <nlohmann/json.hpp>

#include "Core/Log.h"

namespace Sunta
{

void TextureNamingConventionsUtilities::LoadFromFile(const std::string& jsonPath)
{
	std::ifstream file(jsonPath);
	if (!file.is_open())
	{
		SUNTA_ENGINE_LOG_WARNING("TextureNamingConventions::LoadFromFile: Couldn't open '{0}', naming-based texture fallback disabled", jsonPath);
		return;
	}

	nlohmann::json j;

	try
	{
		file >> j;
	}
	catch (const std::exception& error)
	{
		SUNTA_ENGINE_LOG_ERROR("TextureNamingConventions::LoadFromFile: Failed to parse '{0}': {1}", jsonPath, error.what());
		return;
		
	}

	tokensByType.clear();
	for (auto& [type, tokens] : j.items())
		tokensByType[type] = tokens.get<std::vector<std::string>>();

	loaded = true;
	SUNTA_ENGINE_LOG_INFO("TextureNamingConventions::LoadFromFile: Loaded {} texture types.", tokensByType.size());
	
}

std::vector<std::string> TextureNamingConventionsUtilities::TryResolveCandidates(const std::string& diffuseFileName, const std::string& textureType)
{
	std::vector<std::string> candidates;

	if (!loaded)
		return candidates;

	auto diffuseIt = tokensByType.find("diffuse");
	auto targetIt = tokensByType.find(textureType);
	if (diffuseIt == tokensByType.end() || targetIt == tokensByType.end() || targetIt->second.empty())
		return candidates;

	size_t dot = diffuseFileName.find_last_of('.');
	if (dot == std::string::npos)
		return candidates;

	std::string base = diffuseFileName.substr(0, dot);
	std::string extension = diffuseFileName.substr(dot);
	std::string baseLower = ToLower(base);

	for (const auto& diffuseToken : diffuseIt->second)
	{
		std::string diffuseTokenLower = ToLower(diffuseToken);

		bool wholeNameMatches = (baseLower == diffuseTokenLower);
		bool suffixMatches = baseLower.size() >= diffuseTokenLower.size() &&
			baseLower.compare(baseLower.size() - diffuseTokenLower.size(), diffuseTokenLower.size(), diffuseTokenLower) == 0;

		if (!wholeNameMatches && !suffixMatches)
			continue;

		std::string root = wholeNameMatches ? "" : base.substr(0, base.size() - diffuseToken.size());

		for (const auto& targetToken : targetIt->second)
			candidates.push_back(root + targetToken + extension);

		break;
	}

	return candidates;
}

std::string TextureNamingConventionsUtilities::ToLower(const std::string& text)
{
	std::string result = text;
	std::transform(result.begin(), result.end(), result.begin(), ::tolower);
	return result;
}

}