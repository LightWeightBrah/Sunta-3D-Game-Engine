#pragma once
#include <string>

namespace Sunta
{

// Settings of one game, stored in "@game/game.config" (JSON)
// Used by the game build, the editor doesn't need it
struct GameConfig
{
	static constexpr const char* DefaultPath = "@game/game.config";

	std::string  name         = "Sunta Game";
	std::string  startScene   = "";           // virtual path e.g. "@game/Scenes/Main.scene"
	unsigned int windowWidth  = 1600;
	unsigned int windowHeight = 900;

	// Returns false (and leaves outConfig untouched) if the file is missing or broken
	static bool TryLoad(const std::string& virtualPath, GameConfig& outConfig);
};

}
