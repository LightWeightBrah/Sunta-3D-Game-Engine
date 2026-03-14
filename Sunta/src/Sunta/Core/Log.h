#pragma once

#include "Logger.h"

namespace Sunta {

class Log
{
public:
	static void Init();

	static Logger& GetEngineLogger() { return *engineLogger; }
	static Logger& GetGameLogger()	 { return *gameLogger;   }

private:
	static std::unique_ptr<Logger> engineLogger;
	static std::unique_ptr<Logger> gameLogger;
};

}

//we use :: before Sunta to be in global namespace first
#define SUNTA_ENGINE_LOG_INFO(...)     ::Sunta::Log::GetEngineLogger().Info(__VA_ARGS__)
#define SUNTA_ENGINE_LOG_WARNING(...)  ::Sunta::Log::GetEngineLogger().Warning(__VA_ARGS__)
#define SUNTA_ENGINE_LOG_ERROR(...)    ::Sunta::Log::GetEngineLogger().Error(__VA_ARGS__)

#define SUNTA_GAME_LOG_INFO(...)       ::Sunta::Log::GetGameLogger().Info(__VA_ARGS__)
#define SUNTA_GAME_LOG_WARNING(...)    ::Sunta::Log::GetGameLogger().Warning(__VA_ARGS__)
#define SUNTA_GAME_LOG_ERROR(...)      ::Sunta::Log::GetGameLogger().Error(__VA_ARGS__)

#define SUNTA_ENGINE_LOG_SET_LEVEL_INFO       ::Sunta::Log::GetEngineLogger().SetLevel(::Sunta::Logger::Level::ERROR)
#define SUNTA_ENGINE_LOG_SET_LEVEL_WARNING    ::Sunta::Log::GetEngineLogger().SetLevel(::Sunta::Logger::Level::ERROR)
#define SUNTA_ENGINE_LOG_SET_LEVEL_ERROR      ::Sunta::Log::GetEngineLogger().SetLevel(::Sunta::Logger::Level::ERROR)

#define SUNTA_GAME_LOG_SET_LEVEL_INFO         ::Sunta::Log::GetGameLogger().SetLevel(::Sunta::Logger::Level::ERROR)
#define SUNTA_GAME_LOG_SET_LEVEL_WARNING      ::Sunta::Log::GetGameLogger().SetLevel(::Sunta::Logger::Level::ERROR)
#define SUNTA_GAME_LOG_SET_LEVEL_ERROR        ::Sunta::Log::GetGameLogger().SetLevel(::Sunta::Logger::Level::ERROR)
