#include "Core/SuntaPreCompiled.h"
#include "Font.h"

#include <fstream>
#include <iterator>

#include "stb/stb_truetype.h"

#include "RendererDevice.h"
#include "Texture.h"
#include "Core/Log.h"

namespace Sunta
{

namespace
{

// The printable ASCII letters: from space to ~
constexpr int FirstCharacter = 32;
constexpr int CharacterCount = 95;

// Big enough for every letter at the sizes the UI bakes (up to about 100 pixels)
constexpr int AtlasWidth  = 1024;
constexpr int AtlasHeight = 1024;

constexpr unsigned char FullyOpaque = 255;

bool TryReadFile(const std::string& filepath, std::vector<unsigned char>& outBytes)
{
	std::ifstream file(filepath, std::ios::binary);
	if (!file)
		return false;

	outBytes.assign(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
	return !outBytes.empty();
}

// The letters are white, how much a pixel is covered by a letter becomes its transparency (the color is given when drawing)
std::vector<unsigned char> CoverageToWhiteRgba(const std::vector<unsigned char>& coverage)
{
	std::vector<unsigned char> rgba(coverage.size() * 4);

	for (size_t pixel = 0; pixel < coverage.size(); pixel++)
	{
		rgba[pixel * 4 + 0] = FullyOpaque;
		rgba[pixel * 4 + 1] = FullyOpaque;
		rgba[pixel * 4 + 2] = FullyOpaque;
		rgba[pixel * 4 + 3] = coverage[pixel];
	}

	return rgba;
}

float GetAscent(const std::vector<unsigned char>& fontData, float pixelHeight)
{
	stbtt_fontinfo info;
	if (!stbtt_InitFont(&info, fontData.data(), stbtt_GetFontOffsetForIndex(fontData.data(), 0)))
		return pixelHeight;

	int ascent = 0, descent = 0, lineGap = 0;
	stbtt_GetFontVMetrics(&info, &ascent, &descent, &lineGap);

	return ascent * stbtt_ScaleForPixelHeight(&info, pixelHeight);
}

}

bool Font::TryLoad(RendererDevice& rendererDevice, const std::string& filepath, float bakedPixelHeight)
{
	std::vector<unsigned char> fontData;
	if (!TryReadFile(filepath, fontData))
	{
		SUNTA_ENGINE_LOG_ERROR("Font: couldn't read '{0}'", filepath);
		return false;
	}

	std::vector<unsigned char> coverage(AtlasWidth * AtlasHeight);
	std::vector<stbtt_bakedchar> bakedLetters(CharacterCount);

	const int result = stbtt_BakeFontBitmap(fontData.data(), 0, bakedPixelHeight, coverage.data(), AtlasWidth, AtlasHeight,
		FirstCharacter, CharacterCount, bakedLetters.data());

	if (result == 0)
	{
		SUNTA_ENGINE_LOG_ERROR("Font: '{0}' is not a font stb_truetype can read", filepath);
		return false;
	}

	if (result < 0)
		SUNTA_ENGINE_LOG_WARNING("Font: only {0} letters of '{1}' fit at {2} pixels", -result, filepath, bakedPixelHeight);

	letters.clear();
	for (const stbtt_bakedchar& baked : bakedLetters)
	{
		letters.push_back({ glm::vec2(baked.x0, baked.y0), glm::vec2(baked.x1, baked.y1),
			glm::vec2(baked.xoff, baked.yoff), baked.xadvance });
	}

	this->bakedPixelHeight = bakedPixelHeight;
	atlasSize = glm::vec2(AtlasWidth, AtlasHeight);
	ascent    = GetAscent(fontData, bakedPixelHeight);
	texture   = rendererDevice.CreateTexture(AtlasWidth, AtlasHeight, CoverageToWhiteRgba(coverage).data());

	return true;
}

const Font::Letter* Font::FindLetter(char character) const
{
	const int index = static_cast<unsigned char>(character) - FirstCharacter;
	return (index >= 0 && index < static_cast<int>(letters.size())) ? &letters[index] : nullptr;
}

std::vector<Font::LetterQuad> Font::LayOut(const std::string& text, const glm::vec2& topLeft, float pixelHeight) const
{
	std::vector<LetterQuad> quads;
	if (!IsLoaded())
		return quads;

	const float scale    = pixelHeight / bakedPixelHeight;
	const float baseline = topLeft.y + ascent * scale;
	float penX = topLeft.x;

	for (char character : text)
	{
		const Letter* letter = FindLetter(character);
		if (!letter)
			continue;

		const glm::vec2 atlasSizeOfLetter = letter->atlasMax - letter->atlasMin;

		LetterQuad quad;
		quad.position    = glm::vec2(penX + letter->offset.x * scale, baseline + letter->offset.y * scale);
		quad.size        = atlasSizeOfLetter * scale;
		quad.textureArea = glm::vec4(letter->atlasMin / atlasSize, atlasSizeOfLetter / atlasSize);
		quads.push_back(quad);

		penX += letter->advance * scale;
	}

	return quads;
}

float Font::MeasureWidth(const std::string& text, float pixelHeight) const
{
	float width = 0.0f;

	for (char character : text)
	{
		if (const Letter* letter = FindLetter(character))
			width += letter->advance;
	}

	return width * pixelHeight / bakedPixelHeight;
}

}
