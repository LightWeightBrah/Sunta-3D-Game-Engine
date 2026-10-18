#include <exception>

#include <Sunta/Core/Application.h>
#include <Sunta/Core/FatalError.h>
#include <Sunta/Runtime/RuntimeLayer.h>

namespace
{

void RunGame()
{
	Sunta::ApplicationSpecification specification;
	specification.assetSource    = Sunta::AssetSource::PackagedNextToExecutable;
	specification.loadGameConfig = true;

	// A Release game has no console window, so its log goes to a file next to it ("MyGame.log")
	specification.logFilePath = Sunta::VirtualFileSystem::GetExecutablePath().replace_extension(".log");

#if defined(SUNTA_DEVELOPMENT)
	// Started from the IDE (no "res" next to the executable): reads the project from the source tree
	specification.projectDirectory = SUNTA_DEFAULT_PROJECT_DIR;
#endif

	Sunta::Application application(specification);
	application.PushLayer<Sunta::RuntimeLayer>();
	application.Run();
}

}

int main()
{
	// An exception nobody handles would close the game without a word
	try
	{
		RunGame();
	}
	catch (const std::exception& exception)
	{
		Sunta::FatalError::Report(std::string("The game stopped because of an error: ") + exception.what());
	}

	return 0;
}
