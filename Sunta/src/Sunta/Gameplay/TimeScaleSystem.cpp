#include "Core/SuntaPreCompiled.h"
#include "TimeScaleSystem.h"

#include <glm/glm.hpp>

namespace Sunta
{

namespace
{

// A hit stop is not a full stop: the time crawls, so nothing divides by a zero frame time
constexpr float HitStopScale = 0.03f;

// The longest a hit stop or a slow motion may last (a mistake in a scene can't freeze the game)
constexpr float LongestHitStopSeconds    = 0.5f;
constexpr float LongestSlowMotionSeconds = 10.0f;

float hitStopSecondsLeft    = 0.0f;
float slowMotionSecondsLeft = 0.0f;
float slowMotionSeconds     = 0.0f;
float slowMotionScale       = 1.0f;
float currentScale          = 1.0f;

// The slow motion's scale now: full at first, back to normal over its last third
float GetSlowMotionScale()
{
	if (slowMotionSecondsLeft <= 0.0f)
		return 1.0f;

	const float easeOutSeconds = slowMotionSeconds / 3.0f;
	if (slowMotionSecondsLeft >= easeOutSeconds)
		return slowMotionScale;

	const float t = 1.0f - slowMotionSecondsLeft / easeOutSeconds;
	return glm::mix(slowMotionScale, 1.0f, t * t * (3.0f - 2.0f * t));
}

}

void TimeScaleSystem::HitStop(float seconds)
{
	hitStopSecondsLeft = glm::max(hitStopSecondsLeft, glm::clamp(seconds, 0.0f, LongestHitStopSeconds));
}

void TimeScaleSystem::SlowMotion(float seconds, float scale)
{
	slowMotionSeconds     = glm::clamp(seconds, 0.0f, LongestSlowMotionSeconds);
	slowMotionSecondsLeft = slowMotionSeconds;
	slowMotionScale       = glm::clamp(scale, HitStopScale, 1.0f);
}

float TimeScaleSystem::Update(float realDeltaTime)
{
	currentScale = hitStopSecondsLeft > 0.0f ? HitStopScale : GetSlowMotionScale();

	hitStopSecondsLeft    = glm::max(hitStopSecondsLeft    - realDeltaTime, 0.0f);
	slowMotionSecondsLeft = glm::max(slowMotionSecondsLeft - realDeltaTime, 0.0f);

	return realDeltaTime * currentScale;
}

void TimeScaleSystem::Reset()
{
	hitStopSecondsLeft    = 0.0f;
	slowMotionSecondsLeft = 0.0f;
	currentScale          = 1.0f;
}

float TimeScaleSystem::GetScale()
{
	return currentScale;
}

}
