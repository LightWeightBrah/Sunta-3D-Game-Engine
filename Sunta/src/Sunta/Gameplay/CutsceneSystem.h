#pragma once

#include <string>
#include <glm/glm.hpp>

namespace Sunta
{

class EntityManager;

// Plays the cutscenes (see CutsceneComponent): moves the active camera along the cutscene's points, starts its animation,
// draws the black bars and the title, and tells the other systems that the fight waits
class CutsceneSystem
{
public:
	// Starts the cutscene of the entity (once: a cutscene that already played is not started again)
	static void Play(EntityManager& entityManager, unsigned int cutsceneEntityID);

	// Starts the cutscene of the entity with this name (Tag), false when there is none
	static bool PlayByName(EntityManager& entityManager, const std::string& name);

	// True while some cutscene plays: enemies don't think and the player's keys do nothing
	static bool IsPlaying(EntityManager& entityManager);

	// Call once per frame AFTER the follow camera: the cutscene's camera wins
	static void Update(EntityManager& entityManager, float deltaTime);

	// Call last when drawing: the black bars and the title over the picture
	// 'areaTopLeft' and 'areaSize': where the game is on the screen (the editor shows it in a panel)
	static void DrawOverlay(EntityManager& entityManager, const glm::vec2& screenSize, const glm::vec2& areaTopLeft, const glm::vec2& areaSize);

	// When the game starts: no cutscene played yet
	static void PrepareForPlay(EntityManager& entityManager);
};

}
