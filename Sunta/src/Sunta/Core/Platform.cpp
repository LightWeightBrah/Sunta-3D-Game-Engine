#include "SuntaPreCompiled.h"
#include "Platform.h"

#include "Assert.h"

#if   defined(SUNTA_PLATFORM_WINDOWS)
	#include "Platform/Windows/WindowsPlatform.h"
#elif defined(SUNTA_PLATFORM_LINUX)
	#include "Platform/Linux/LinuxPlatform.h"
#elif defined(SUNTA_PLATFORM_MAC)
	#include "Platform/Mac/MacPlatform.h"
#endif    

namespace Sunta
{

void Platform::Init()
{
	instance = Create();
}

void Platform::Shutdown()
{
	instance.reset();
}

std::unique_ptr<Platform> Platform::Create()
{
#if   defined(SUNTA_PLATFORM_WINDOWS)
	return std::make_unique<WindowsPlatform>();

#elif defined(SUNTA_PLATFORM_LINUX)
	return std::make_unique<LinuxPlatform>();

#elif defined(SUNTA_PLATFORM_MAC)
	return std::make_unique<MacPlatform>();

#else
	SUNTA_ASSERT(false, "Unsupported platform! Sunta Engine does not support this platform yet.");
	return nullptr;

#endif
}

}