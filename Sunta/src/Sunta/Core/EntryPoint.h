#pragma once

#ifdef SUNTA_PLATFORM_WINDOWS

extern Sunta::Application* Sunta::CreateApplication();

int main(int argc, char** argv)
{
	auto application = Sunta::CreateApplication();
	
	application->Run();
	
	delete application;
}

#endif