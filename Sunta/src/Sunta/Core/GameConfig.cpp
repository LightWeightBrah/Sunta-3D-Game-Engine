#include "SuntaPreCompiled.h"
#include "GameConfig.h"

#include <nlohmann/json.hpp>

#include "Log.h"
#include "VirtualFileSystem.h"

namespace Sunta
{

namespace GameConfigKeys
{
	constexpr const char* Name         = "name";
	constexpr const char* StartScene   = "start_scene";
	constexpr const char* WindowWidth  = "window_width";
	constexpr const char* WindowHeight = "window_height";
	constexpr const char* WindowIcon   = "window_icon";
}

bool GameConfig::TryLoad(const std::string& virtualPath, GameConfig& outConfig)
{
	const std::string physicalPath = VirtualFileSystem::Resolve(virtualPath);

	std::ifstream configFile(physicalPath);
	if (!configFile.is_open())
	{
		SUNTA_ENGINE_LOG_ERROR("GameConfig: Couldn't open '{0}'", physicalPath);
		return false;
	}

	try
	{
		nlohmann::json configJson;
		configFile >> configJson;

		GameConfig loadedConfig = outConfig;
		loadedConfig.name         = configJson.value(GameConfigKeys::Name,         loadedConfig.name);
		loadedConfig.startScene   = configJson.value(GameConfigKeys::StartScene,   loadedConfig.startScene);
		loadedConfig.windowWidth  = configJson.value(GameConfigKeys::WindowWidth,  loadedConfig.windowWidth);
		loadedConfig.windowHeight = configJson.value(GameConfigKeys::WindowHeight, loadedConfig.windowHeight);
		loadedConfig.windowIcon   = configJson.value(GameConfigKeys::WindowIcon,   loadedConfig.windowIcon);

		outConfig = loadedConfig;
		return true;
	}
	catch (const nlohmann::json::exception& error)
	{
		SUNTA_ENGINE_LOG_ERROR("GameConfig: '{0}' is not a valid config: {1}", physicalPath, error.what());
		return false;
	}
}


bool GameConfig::TrySave(const std::string& virtualPath, const GameConfig& config)
{
	const std::string physicalPath = VirtualFileSystem::Resolve(virtualPath);

	// ordered_json keeps the keys in the order they are added, so the file stays readable
	nlohmann::ordered_json configJson;
	configJson[GameConfigKeys::Name]         = config.name;
	configJson[GameConfigKeys::StartScene]   = config.startScene;
	configJson[GameConfigKeys::WindowWidth]  = config.windowWidth;
	configJson[GameConfigKeys::WindowHeight] = config.windowHeight;
	configJson[GameConfigKeys::WindowIcon]   = config.windowIcon;

	std::ofstream configFile(physicalPath);
	if (!configFile.is_open())
	{
		SUNTA_ENGINE_LOG_ERROR("GameConfig: Couldn't write '{0}'", physicalPath);
		return false;
	}

	configFile << configJson.dump(1, '\t') << '\n';
	return configFile.good();
}

}
