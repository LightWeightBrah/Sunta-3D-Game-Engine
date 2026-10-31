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

	// Call after the scene is drawn. 'viewProjection' is the camera the scene was drawn with, 'screenSize' the whole window
	// (the marks over enemies follow them on the whole window), 'area' where the bars and the messages go
	static void Draw(EntityManager& entityManager, const glm::mat4& viewProjection, const glm::vec2& screenSize, const HUDArea& area);
};

}
