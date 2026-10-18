#include "Core/SuntaPreCompiled.h"
#include "HeadlessGameBuild.h"

#include "GameBuilder.h"
#include "ProjectManager.h"
#include "Core/Log.h"
#include "Core/VirtualFileSystem.h"

namespace Sunta
{

int HeadlessGameBuild::Run(const std::filesystem::path& projectDirectory, bool isDevelopmentBuild)
{
	Log::Init();

	if (!ProjectManager::IsProjectDirectory(projectDirectory))
	{
		SUNTA_ENGINE_LOG_ERROR("HeadlessGameBuild: '{0}' is not a project folder (it has no 'res' folder). Usage: SuntaEditor --build-game <project folder> [--development]",
							   projectDirectory.string());
		return EXIT_FAILURE;
	}

	VirtualFileSystem::Init(AssetSource::SourceTree, projectDirectory);

	const bool shouldOpenResultFolder = false;
	const bool isBuilt = GameBuilder::BuildGame(isDevelopmentBuild, shouldOpenResultFolder);

	return isBuilt ? EXIT_SUCCESS : EXIT_FAILURE;
}

}
