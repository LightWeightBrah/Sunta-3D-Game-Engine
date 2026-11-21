#pragma once

namespace Sunta
{

// The editor's Console panel: the engine's and the game's log (Lua print()) as it is written, in its colors.
// Shown as a tab next to the File Browser: the editor runs without a console window of its own
class ConsolePanel
{
public:
	static void Draw();

private:
	static inline bool   showsInfo      = true;
	static inline bool   showsWarnings  = true;
	static inline bool   showsErrors    = true;
	static inline bool   followsNewest  = true;
	static inline size_t linesSeen      = 0;
	static inline char   filter[128]    = {};
};

}
