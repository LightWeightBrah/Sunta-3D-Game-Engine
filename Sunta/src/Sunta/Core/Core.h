#pragma once

// ========================= PLATFORMS ========================= 
#if   defined(SUNTA_PLATFORM_WINDOWS)
	//CODE NEEDED FOR WINDOWS
#elif defined(SUNTA_PLATFORM_LINUX)
	//CODE NEEDED FOR LINUX
#elif defined(SUNTA_PLATFORM_MAC)
	//CODE NEEDED FOR MAC
#else
	#error ERROR: Sunta Engine does not support this platform!
#endif
