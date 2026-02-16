#pragma once

#ifdef SUNTA_PLATFORM_WINDOWS
	#ifdef SUNTA_BUILD_DLL
		#define SUNTA_API __declspec(dllexport)
	#else
		#define SUNTA_API __declspec(dllimport)
	#endif
#else
	#error ERROR: Sunta currently supports only Windows
#endif