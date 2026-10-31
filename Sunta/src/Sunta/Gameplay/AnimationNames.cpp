#include "Core/SuntaPreCompiled.h"
#include "AnimationNames.h"

#include <algorithm>
#include <cctype>

#include "Animation/Animation.h"

namespace Sunta
{

namespace
{

std::string ToLowerCase(std::string text)
{
	std::transform(text.begin(), text.end(), text.begin(), [](unsigned char letter) { return static_cast<char>(std::tolower(letter)); });
	return text;
}

}

bool TryFindAnimationContaining(const AnimationMap& animations, const std::string& keyword, std::string& outName)
{
	const std::string lowerCaseKeyword = ToLowerCase(keyword);

	for (const auto& [name, animation] : animations)
	{
		if (ToLowerCase(name).find(lowerCaseKeyword) != std::string::npos)
		{
			outName = name;
			return true;
		}
	}

	return false;
}

}
