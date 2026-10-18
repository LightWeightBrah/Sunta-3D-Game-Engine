#pragma once
#include <algorithm>
#include <atomic>
#include <mutex>
#include <string>

namespace Sunta
{

// How far a game build is: the thread that builds writes it, the window that shows the progress bar reads it
// The build is split into units (one copied file is one unit), it is done when all of them are finished
class BuildProgress
{
public:
	void Reset()
	{
		Start(1);
		SetStep("Starting");
	}

	void Start(size_t newTotalUnits)
	{
		doneUnits  = 0;
		totalUnits = std::max<size_t>(newTotalUnits, 1);
	}

	void Advance()
	{
		++doneUnits;
	}

	void SetStep(const std::string& newStep)
	{
		std::lock_guard<std::mutex> lock(stepMutex);
		step = newStep;
	}

	// 0 (nothing done) to 1 (everything done)
	float GetFraction() const
	{
		const float fraction = static_cast<float>(doneUnits.load()) / static_cast<float>(totalUnits.load());
		return std::min(fraction, 1.0f);
	}

	std::string GetStep() const
	{
		std::lock_guard<std::mutex> lock(stepMutex);
		return step;
	}

private:
	std::atomic<size_t> doneUnits  { 0 };
	std::atomic<size_t> totalUnits { 1 };

	mutable std::mutex stepMutex;
	std::string        step;
};

}
