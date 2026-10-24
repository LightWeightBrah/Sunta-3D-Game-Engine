#pragma once
#include <filesystem>
#include <string>

#include "Sunta/Physics/CollisionSettings.h"

namespace Sunta
{

// Settings of one game, stored in "@game/game.config" (JSON)
// Read by the game and by the editor (Project Settings window edits it)
struct GameConfig
{
	static constexpr const char* DefaultPath = "@game/game.config";

	std::string  name         = "Sunta Game";
	std::string  startScene   = "";           // virtual path e.g. "@game/Scenes/Main.scene"
	unsigned int windowWidth  = 1600;
	unsigned int windowHeight = 900;
	std::string  windowIcon   = "@game/App/window_icon.png"; // optional: the engine icon is used if the file doesn't exist
	bool         fullscreen   = false;         // the game covers the whole main screen (the window size above is used when it is off)
	bool         vsync        = true;          // a frame is shown when the screen refreshes: no tearing, no wasted work (see Window::SetVSync)

	// Which collision layers the project has and which of them collide (edited in the editor: Project > Collision Layers & Matrix...),
	// the defaults are used when the file has none
	CollisionSettings collisionSettings;

	// Set only in the copy of the config inside a built development game (see GameBuilder), never in the project itself:
	// the game opens a console window, shows its log there and reloads changed scripts. Written to the file only when true
	bool         developerMode = false;

	// Returns false (and leaves outConfig untouched) if the file is missing or broken
	static bool TryLoad(const std::string& virtualPath, GameConfig& outConfig);

	// Writes the config to the file, replacing the old one. Returns false if the file can't be written
	static bool TrySave(const std::string& virtualPath, const GameConfig& config);

	// Same, for a file that is not inside the mounted folders (e.g. the config of a game being built)
	static bool TrySaveToFile(const std::filesystem::path& filePath, const GameConfig& config);
};

}
