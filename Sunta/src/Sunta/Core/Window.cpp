#include "SuntaPreCompiled.h"
#include "Window.h"

#include "Assert.h"

#if   defined(SUNTA_PLATFORM_WINDOWS)
    #include "Platform/Windows/WindowsWindow.h"
#elif defined(SUNTA_PLATFORM_LINUX)
    #include "Platform/Linux/LinuxWindow.h"
#elif defined(SUNTA_PLATFORM_MACOS)
    #include "Platform/MacOS/MacOSWindow.h"
#endif    

namespace Sunta
{

std::unique_ptr<Window> Window::CreateWindow(const std::string& title, int width, int height)
{
    #if   defined(SUNTA_PLATFORM_WINDOWS)
        return std::make_unique<WindowsWindow>(title, width, height);

    #elif defined(SUNTA_PLATFORM_LINUX)
        return std::make_unique<LinuxWindow>(title, width, height);

    #elif defined(SUNTA_PLATFORM_MACOS)
        return std::make_unique<MacWindow>(title, width, height);

    #else
        SUNTA_ASSERT(false, "Unsupported platform! Sunta Engine does not support this platform yet.");
        return nullptr;

    #endif
}

}