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

		outConfig = loadedConfig;
		return true;
	}
	catch (const nlohmann::json::exception& error)
	{
		SUNTA_ENGINE_LOG_ERROR("GameConfig: '{0}' is not a valid config: {1}", physicalPath, error.what());
		return false;
	}
}

}
