#pragma once

#include <string>
#include <glm/glm.hpp>

namespace Sunta
{

class EntityManager;

// The part of the screen the HUD is laid out in (pixels): the bars sit in its corners, the death message in its middle
// In a game it is the whole window, in the editor the space between the editor's panels
struct HUDArea
{
	glm::vec2 topLeft = glm::vec2(0.0f);
	glm::vec2 size    = glm::vec2(0.0f);
};

// Draws the HUD of the game: the player's bars, the lock-on marker, the enemies' health bars and the death message
// (see PlayerHUDComponent and EnemyHealthBarComponent)
class HUDSystem
{
public:
	// Call when the game starts: the bars start full, nothing was hit yet
	static void PrepareForPlay(EntityManager& entityManager);

	// Call once per frame while the game plays: the slowly shrinking part of the bars, the timers
	static void Update(EntityManager& entityManager, float deltaTime);

	// What the player can do right now, drawn at the bottom of the screen this frame (call it every frame while it applies)
	static void ShowPrompt(EntityManager& entityManager, const std::string& text);

	// A big message in the middle of the screen that fades away after a few seconds
	static void ShowNotification(EntityManager& entityManager, const std::string& text);

	// A text read to the player (a book, a letter) on a panel for 'seconds': its title and its words, wrapped
	static void ShowReading(EntityManager& entityManager, const std::string& title, const std::string& text, float seconds);

	// A won boss fight: a gold banner across the screen, after 'afterSeconds' and once no cutscene plays
	static void ShowVictory(EntityManager& entityManager, const std::string& text, float afterSeconds);

	// The whole screen turns to a color and fades back in 'seconds' (a blinding explosion, a blackout).
	// 'holdSeconds' keeps it full before it starts to fade. Drawn over everything, also over a cutscene
	// 'strength' is how much it covers at its fullest: 1 = the color alone (a white screen), 0.35 = a blink of light
	// over the picture that is still seen through it
	static void FlashScreen(const glm::vec3& color, float seconds, float holdSeconds = 0.0f, float strength = 1.0f);

	// The sign of a blow that can't be blocked (Sekiro's perilous kanji): a red mark in the middle of the screen that
	// swells in and fades within a second, drawn with the screen flash
	static void ShowPerilousSign();
	static void DrawScreenFlash(const glm::vec2& screenSize);

	// Call after the scene is drawn. 'viewProjection' is the camera the scene was drawn with, 'screenSize' the whole window
	// (the marks over enemies follow them on the whole window), 'area' where the bars and the messages go
	static void Draw(EntityManager& entityManager, const glm::mat4& viewProjection, const glm::vec2& screenSize, const HUDArea& area);
};

}
