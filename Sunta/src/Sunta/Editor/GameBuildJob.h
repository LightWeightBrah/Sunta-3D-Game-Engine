#pragma once
#include <atomic>
#include <thread>

#include "BuildProgress.h"
#include "GameBuilder.h"

namespace Sunta
{

// One game build running on its own thread, so the editor stays alive while files are copied
// Use from one thread only (the one that draws the editor): Start(), then ask IsFinished() every frame, then TakeResult()
class GameBuildJob
{
public:
	GameBuildJob() = default;
	~GameBuildJob();

	GameBuildJob(const GameBuildJob&)            = delete;
	GameBuildJob& operator=(const GameBuildJob&) = delete;

	// Does nothing when a build is already running
	void Start(bool isDevelopmentBuild);

	// True from Start() until TakeResult()
	bool IsBuilding() const { return isActive; }

	// The thread has ended, the result is waiting to be taken
	bool IsFinished() const { return isActive && isThreadDone; }

	const BuildProgress& GetProgress() const { return progress; }

	// Only after IsFinished()
	BuildResult TakeResult();

	// Blocks until the build ends (the editor must not close while files are being copied)
	void WaitUntilFinished();

private:
	std::thread       worker;
	std::atomic<bool> isThreadDone { false };
	bool              isActive     = false;

	BuildProgress progress;
	BuildResult   result;
};

}
