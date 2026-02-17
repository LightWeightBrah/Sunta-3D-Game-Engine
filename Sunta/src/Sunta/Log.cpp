#include "Log.h"

namespace Sunta {

std::unique_ptr<Logger> Log::engineLogger;
std::unique_ptr<Logger> Log::gameLogger;

void Log::Init()
{
	Terminal::Init();

	engineLogger = std::make_unique<Logger>("SUNTA");
	gameLogger   = std::make_unique<Logger>("GAME");
}

}