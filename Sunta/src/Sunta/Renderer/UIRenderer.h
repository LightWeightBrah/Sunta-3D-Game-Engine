#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <glm/glm.hpp>

namespace Sunta
{

class Font;
class Shader;
class Texture;
class VertexArray;

// Draws the game's UI (the HUD): flat rectangles and text over the 3D scene, in pixels of the screen
// 0,0 is the top left corner of the screen, x goes right, y goes down
//
// Everything between Begin and End is drawn on top of the scene, in the order it is called (later things cover earlier ones)
// The shader and the font are loaded the first time something is drawn, so a program that never draws UI loads nothing
class UIRenderer
{
public:
	static void Begin(const glm::vec2& screenSize);
	static void End();

	static glm::vec2 GetScreenSize() { return screenSize; }

	static void DrawRectangle(const glm::vec2& topLeft, const glm::vec2& size, const glm::vec4& color);
	// A picture (e.g. a frame of a video) stretched over the rectangle, its colors multiplied by 'tint'
	static void DrawImage(const glm::vec2& topLeft, const glm::vec2& size, const Texture& texture, const glm::vec4& tint = glm::vec4(1.0f));

	// A part of a picture: 'area' is u, v, width, height in 0..1 of the texture (v = 0 is the top row)
	static void DrawImageArea(const glm::vec2& topLeft, const glm::vec2& size, const Texture& texture, const glm::vec4& area,
		const glm::vec4& tint = glm::vec4(1.0f));

	// A frame that can be any size (nine-slice): the corners of the picture keep their shape, the edges stretch along
	// their length and the middle stretches both ways. 'borders' is left, top, right, bottom in pixels of the texture,
	// 'borderScale' how big one texture pixel of the borders is on the screen. A border of 0 makes that side stretch
	// (left/right only = three-slice, e.g. a bar whose ornate ends keep their shape however long the bar grows)
	static void DrawNineSlice(const glm::vec2& topLeft, const glm::vec2& size, const Texture& texture, const glm::vec4& borders,
		float borderScale, const glm::vec4& tint = glm::vec4(1.0f));

	// A picture for the UI from a file (@game/... or @engine/...), loaded once and kept. nullptr when it can't be read
	static std::shared_ptr<Texture> GetImage(const std::string& path);

	static void DrawRectangleOutline(const glm::vec2& topLeft, const glm::vec2& size, float thickness, const glm::vec4& color);

	// Letters 'pixelHeight' tall with a soft dark shadow, so the text can be read over a bright scene
	static void DrawTextAt(const std::string& text, const glm::vec2& topLeft, float pixelHeight, const glm::vec4& color);
	static void DrawTextCentered(const std::string& text, const glm::vec2& center, float pixelHeight, const glm::vec4& color);
	static float MeasureText(const std::string& text, float pixelHeight);

	// Where a point of the world is on the screen (pixels). False when it is behind the camera
	static bool TryProjectToScreen(const glm::vec3& worldPoint, const glm::mat4& viewProjection, glm::vec2& outPixel);

private:
	static inline std::shared_ptr<VertexArray> quad;
	static inline std::shared_ptr<Shader>      shader;
	static inline std::shared_ptr<Font>        font;
	static inline glm::vec2                    screenSize = glm::vec2(0.0f);
	static inline std::unordered_map<std::string, std::shared_ptr<Texture>> images;

	static void EnsureInitialized();
	static void CreateQuad();
	static void DrawQuad(const glm::vec2& topLeft, const glm::vec2& size, const glm::vec4& color, const Texture* texture, const glm::vec4& textureArea);
	static void DrawLetters(const std::string& text, const glm::vec2& topLeft, float pixelHeight, const glm::vec4& color);
};

}
