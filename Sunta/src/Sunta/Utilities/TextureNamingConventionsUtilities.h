#pragma once
#include <string>
#include <vector>
#include <unordered_map>

namespace Sunta
{

class TextureNamingConventionsUtilities
{
public:
	static void LoadFromFile(const std::string& jsonPath);

	static std::vector<std::string> TryResolveCandidates(const std::string& diffuseFileName, const std::string& textureType);

private:
	static inline std::unordered_map<std::string, std::vector<std::string>> tokensByType;
	static inline bool loaded = false;

	static std::string ToLower(const std::string& text);


};

}