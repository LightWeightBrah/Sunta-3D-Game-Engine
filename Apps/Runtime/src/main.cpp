#include <Sunta/Core/Application.h>
#include <Sunta/Runtime/RuntimeLayer.h>

int main()
{
	Sunta::ApplicationSpecification specification;
	specification.assetSource    = Sunta::AssetSource::PackagedNextToExecutable;
	specification.loadGameConfig = true;

#if defined(SUNTA_DEVELOPMENT)
	// Started from the IDE (no "res" next to the executable): reads the project from the source tree
	specification.projectDirectory = SUNTA_DEFAULT_PROJECT_DIR;
#endif

	Sunta::Application application(specification);
	application.PushLayer<Sunta::RuntimeLayer>();
	application.Run();

	return 0;
}
