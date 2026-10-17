#include <Sunta/Core/Application.h>
#include <Sunta/Editor/EditorLayer.h>

// Usage: SuntaEditor.exe [path to a game project folder]
// Without the argument it opens the project chosen in CMake (SUNTA_PROJECT)
int main(int argc, char** argv)
{
	const bool hasProjectArgument = argc > 1;

	Sunta::ApplicationSpecification specification;
	specification.windowTitle      = "Sunta Editor";
	specification.assetSource      = Sunta::AssetSource::SourceTree;
	specification.projectDirectory = hasProjectArgument ? argv[1] : SUNTA_DEFAULT_PROJECT_DIR;

	Sunta::Application application(specification);
	application.PushLayer<Sunta::EditorLayer>();
	application.Run();

	return 0;
}
