#pragma once

#if defined(SUNTA_PLATFORM_WINDOWS) || defined(SUNTA_PLATFORM_LINUX) || defined(SUNTA_PLATFORM_MAC)

extern Sunta::Application* Sunta::CreateApplication();

int main(int argc, char** argv)
{
	auto application = Sunta::CreateApplication();
	
	application->Run();
	
	delete application;

	return 0;
}

#endif