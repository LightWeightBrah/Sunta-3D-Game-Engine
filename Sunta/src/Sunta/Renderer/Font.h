#pragma once

#include <memory>
#include <string>
#include <vector>
#include <glm/glm.hpp>

namespace Sunta
{

class Texture;
class RendererDevice;

// The letters of a TrueType font drawn once into a texture, so text can be drawn as one rectangle per letter
// Only the printable ASCII letters are kept (space to ~). The text can be drawn at any size, it is scaled from 'bakedPixelHeight'
class Font
{
public:
	// Where one letter goes on the screen and which part of the font texture it shows
	struct LetterQuad
	{
		glm::vec2 position;    // top left corner, pixels
		glm::vec2 size;        // pixels
		glm::vec4 textureArea; // u, v, width, height
	};

	// False (and an error in the log) when the file can't be read or isn't a font
	bool TryLoad(RendererDevice& rendererDevice, const std::string& filepath, float bakedPixelHeight);

	bool IsLoaded() const { return texture != nullptr; }
	const Texture& GetTexture() const { return *texture; }

	// The letters of the text, with the top of the line at 'topLeft' and the letters 'pixelHeight' tall
	std::vector<LetterQuad> LayOut(const std::string& text, const glm::vec2& topLeft, float pixelHeight) const;

	float MeasureWidth(const std::string& text, float pixelHeight) const;

private:
	struct Letter
	{
		glm::vec2 atlasMin;  // pixels in the texture
		glm::vec2 atlasMax;
		glm::vec2 offset;    // from the pen position (on the baseline) to the top left corner, at the baked size
		float     advance;   // how far the pen moves after this letter, at the baked size
	};

	std::shared_ptr<Texture> texture;
	std::vector<Letter> letters;
	glm::vec2 atlasSize = glm::vec2(0.0f);
	float bakedPixelHeight = 0.0f;
	float ascent = 0.0f; // from the top of the line to the baseline, at the baked size

	const Letter* FindLetter(char character) const;
};

}
