#pragma once

//PLATFORM DETECTION
#ifdef _WIN32
	#define SUNTA_PLATFORM_WINDOWS
#endif

//DLL EXPORT/IMPORT
#ifdef SUNTA_PLATFORM_WINDOWS
	#ifdef SUNTA_BUILD_DLL
		#define SUNTA_API __declspec(dllexport)
	#else
		#define SUNTA_API __declspec(dllimport)
	#endif
#else
	#error ERROR: Sunta currently supports only Windows
#endif

//CREATE APPLICATION - MEANT TO BE USED BY CLIENT
#define SUNTA_NEW_APPLICATION(game_class) \
	Sunta::Application* Sunta::CreateApplication() \
	{ \
		return new game_class(); \
	}