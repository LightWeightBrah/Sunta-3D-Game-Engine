#include "Core/SuntaPreCompiled.h"
#include "GameBuildJob.h"

namespace Sunta
{

GameBuildJob::~GameBuildJob()
{
	WaitUntilFinished();
}

void GameBuildJob::Start(bool isDevelopmentBuild)
{
	if (isActive)
		return;

	progress.Reset();
	isThreadDone = false;
	isActive     = true;

	worker = std::thread([this, isDevelopmentBuild]()
		{
			result       = GameBuilder::BuildGame(isDevelopmentBuild, &progress);
			isThreadDone = true;
		});
}

BuildResult GameBuildJob::TakeResult()
{
	WaitUntilFinished();

	isActive = false;
	return result;
}

void GameBuildJob::WaitUntilFinished()
{
	if (worker.joinable())
		worker.join();
}

}
