#pragma once

#include <glm/glm.hpp>

namespace Sunta
{

// The pause menu of a running game: Escape stops the game's time (and its sounds) and shows "Resume" and "Quit".
// Escape again, "Resume" or clicking it goes back to the game. "Quit" is only asked for here: the layer that runs the
// game decides what it means (a built game closes, the editor stops Play mode), see TakeQuitRequest.
// Chosen with the mouse, or with W/S (the arrows) and Enter (Space)
class PauseMenu
{
public:
	// Call once per frame while the game plays, before the game updates: Escape and the menu's keys
	// 'isEscapeTaken': something else answers Escape this frame (a player resting at a bonfire stands up with it,
	// as a menu of the bonfire is left), so it doesn't pause
	static void Update(bool isEscapeTaken = false);

	static bool IsPaused() { return isPaused; }

	// Draws the darkened screen and the menu over the game (after the HUD). 'areaTopLeft' and 'areaSize' are where
	// the game is on the screen (in the editor: its viewport), in the same pixels as the mouse
	static void Draw(const glm::vec2& areaTopLeft, const glm::vec2& areaSize);

	// True once after "Quit" was chosen
	static bool TakeQuitRequest();

	// When the game starts or stops: not paused, the sounds running
	static void Reset();

private:
	static void SetPaused(bool paused);

	static inline bool isPaused        = false;
	static inline bool isQuitRequested = false;
	static inline int  selected        = 0;

	// Where the buttons were drawn last frame (the mouse is checked against them)
	static inline glm::vec2 buttonTopLeft[2] = {};
	static inline glm::vec2 buttonSize       = glm::vec2(0.0f);
};

}
