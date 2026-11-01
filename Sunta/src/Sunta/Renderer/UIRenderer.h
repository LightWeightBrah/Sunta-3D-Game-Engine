#pragma once

#include <memory>
#include <string>
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
	// A picture (e.g. a frame of a video) stretched over the rectangle
	static void DrawImage(const glm::vec2& topLeft, const glm::vec2& size, const Texture& texture);

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

	static void EnsureInitialized();
	static void CreateQuad();
	static void DrawQuad(const glm::vec2& topLeft, const glm::vec2& size, const glm::vec4& color, const Texture* texture, const glm::vec4& textureArea);
	static void DrawLetters(const std::string& text, const glm::vec2& topLeft, float pixelHeight, const glm::vec4& color);
};

}
