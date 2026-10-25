#pragma once

#include <cmath>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace Sunta
{

// Small calculations shared by the gameplay systems (character, lock-on, camera)
//
// Angles are in degrees. "Yaw" is the turn around the up axis: 0 looks along -Z, a positive yaw turns to the left
// "Pitch" is how far a direction looks DOWN: 0 is level, a positive pitch looks at the ground
namespace GameplayMath
{

// Anything shorter than this is treated as "no direction at all" (a direction can't be made of nothing)
constexpr float SmallestUsableLength = 0.0001f;

inline const glm::vec3 WorldUp        = glm::vec3(0.0f, 1.0f,  0.0f);
inline const glm::vec3 DefaultForward = glm::vec3(0.0f, 0.0f, -1.0f);

inline glm::vec3 KeepOnlyHorizontalPart(const glm::vec3& vector)
{
	return glm::vec3(vector.x, 0.0f, vector.z);
}

inline glm::vec3 GetHorizontalDirectionOrDefault(const glm::vec3& direction)
{
	const glm::vec3 horizontalPart = KeepOnlyHorizontalPart(direction);

	if (glm::length(horizontalPart) < SmallestUsableLength)
		return DefaultForward;

	return glm::normalize(horizontalPart);
}

// Angles are kept between -180 and 180 degrees, so "turn left or right?" always has a short answer
inline float WrapDegrees(float angle)
{
	return glm::mod(angle + 180.0f, 360.0f) - 180.0f;
}

// The yaw that makes an entity look along the given direction (the height of the direction is ignored)
// An entity looks along its -Z axis when the yaw is 0
inline float GetYawToFace(const glm::vec3& direction)
{
	return glm::degrees(std::atan2(-direction.x, -direction.z));
}

// How far the direction looks down (negative: up). The direction doesn't have to be normalized
inline float GetPitchOf(const glm::vec3& direction)
{
	const float length = glm::length(direction);

	if (length < SmallestUsableLength)
		return 0.0f;

	return glm::degrees(std::asin(glm::clamp(-direction.y / length, -1.0f, 1.0f)));
}

// The opposite of GetYawToFace and GetPitchOf: the direction (length 1) that has the given yaw and pitch
inline glm::vec3 GetDirectionFromYawAndPitch(float yawDegrees, float pitchDegrees)
{
	const float yaw   = glm::radians(yawDegrees);
	const float pitch = glm::radians(pitchDegrees);

	return glm::vec3(-std::sin(yaw) * std::cos(pitch), -std::sin(pitch), -std::cos(yaw) * std::cos(pitch));
}

// How much of the remaining way is covered in one frame by something that catches up with its goal
// It doesn't depend on the frame rate: two frames of 8 ms give the same result as one of 16 ms
// 'sharpness' is how eager the catching up is (higher = faster, 0 = never arrives)
inline float GetSmoothingFactor(float sharpness, float deltaTime)
{
	return 1.0f - std::exp(-glm::max(sharpness, 0.0f) * deltaTime);
}

inline float SmoothValue(float current, float wanted, float sharpness, float deltaTime)
{
	return current + (wanted - current) * GetSmoothingFactor(sharpness, deltaTime);
}

inline glm::vec3 SmoothPoint(const glm::vec3& current, const glm::vec3& wanted, float sharpness, float deltaTime)
{
	return current + (wanted - current) * GetSmoothingFactor(sharpness, deltaTime);
}

// The same for angles: goes the short way round (from 170 to -170 degrees it turns 20 degrees, not 340)
inline float SmoothAngle(float current, float wanted, float sharpness, float deltaTime)
{
	return WrapDegrees(current + WrapDegrees(wanted - current) * GetSmoothingFactor(sharpness, deltaTime));
}

}

}
