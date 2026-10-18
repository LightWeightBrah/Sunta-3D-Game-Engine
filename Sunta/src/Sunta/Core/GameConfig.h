#pragma once
#include <filesystem>
#include <string>

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

	// Set only in the copy of the config inside a built development game (see GameBuilder), never in the project itself:
	// the game opens a console window and shows its log there. Written to the file only when true
	bool         developerMode = false;

	// Returns false (and leaves outConfig untouched) if the file is missing or broken
	static bool TryLoad(const std::string& virtualPath, GameConfig& outConfig);

	// Writes the config to the file, replacing the old one. Returns false if the file can't be written
	static bool TrySave(const std::string& virtualPath, const GameConfig& config);

	// Same, for a file that is not inside the mounted folders (e.g. the config of a game being built)
	static bool TrySaveToFile(const std::filesystem::path& filePath, const GameConfig& config);
};

}
