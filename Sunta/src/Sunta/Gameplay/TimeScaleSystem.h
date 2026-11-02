#pragma once

namespace Sunta
{

// How fast the game's time runs: a short freeze when a heavy blow lands (hit stop) and slow motion for big moments
// (the killing blow on a boss). Everything that plays (animations, movement, enemies, particles) gets the scaled time,
// the camera, the HUD and the sounds keep the real one, so the camera still shakes during a freeze
class TimeScaleSystem
{
public:
	// The game time stands almost still for 'seconds' (real seconds). A longer freeze already going on is kept
	static void HitStop(float seconds);

	// The game time runs at 'scale' (0..1) for 'seconds' (real seconds), easing back to normal over the last third
	static void SlowMotion(float seconds, float scale);

	// Call once per frame with the real frame time: returns the game's frame time
	static float Update(float realDeltaTime);

	// When the game starts and stops: normal speed
	static void Reset();

	static float GetScale();
};

}
