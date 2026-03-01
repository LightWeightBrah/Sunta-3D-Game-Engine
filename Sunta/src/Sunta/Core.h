#pragma once

#include "PlatformDetection.h"

// =========== PLATFORMS ===========
#ifdef SUNTA_PLATFORM_WINDOWS
	//CODE NEEDED FOR WINDOWS
#else
	#error ERROR: Sunta currently supports only Windows
#endif

//=========== CREATE APPLICATION - MEANT TO BE USED BY CLIENT ===========
#define SUNTA_NEW_APPLICATION(game_class) \
	Sunta::Application* Sunta::CreateApplication() \
	{ \
		return new game_class(); \
	}