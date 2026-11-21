#pragma once

#include <string>

namespace Sunta
{

class Window;

// A loading screen while the engine, a project or a game loads, instead of waiting at an empty window: the name of
// what loads, a bar and the step it is at. Long loads report their progress with Step (from the loop that loads, on
// the main thread); the screen is drawn at most 30 times a second, so it costs the load next to nothing
//
// The window shows up with it (a background test run keeps its window hidden, see BackgroundRun.h)
class LoadingScreen
{
public:
	static void Begin(Window& window, const std::string& title);

	// 'progress' 0..1 of the whole load; 'label' what is loading now (a file, "Starting the game")
	static void Step(float progress, const std::string& label);

	// Gone: the first frame of the program is drawn over it
	static void End();

	// Drawn again over a frame the program has just drawn (shown at the next swap): the first frames of the program,
	// still being laid out, are not seen
	static void Cover();

	static bool IsActive() { return window != nullptr; }

private:
	static void Draw(bool present = true);

	static inline Window*     window        = nullptr;
	static inline std::string title;
	static inline std::string label;
	static inline float       progress      = 0.0f;
	static inline double      lastDrawnAt   = -1.0;
};

}
