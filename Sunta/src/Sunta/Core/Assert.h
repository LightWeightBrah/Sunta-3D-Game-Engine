#pragma once

#include "Log.h"

#ifdef SUNTA_DEBUG
	#define SUNTA_DEBUGBREAK() __debugbreak()
	
	//__VA_ARGS__ pastes everything what we passed as arguments in ... 
	#define SUNTA_ASSERT(x, ...) \
	{ \
		if(!(x)) \
		{ \
			SUNTA_ENGINE_LOG_ERROR("Assertion failed: {0} |  File: {1} Line: {2}", __VA_ARGS__, __FILE__, __LINE__); \
			SUNTA_DEBUGBREAK(); \
		} \
	}
#else //NO ASSERT IN RELEASE MODE
	#define SUNTA_DEBUGBREAK()
	#define SUNTA_ASSERT(x, ...)
#endif