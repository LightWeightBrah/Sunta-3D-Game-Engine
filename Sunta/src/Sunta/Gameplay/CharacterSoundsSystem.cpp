#include "Core/SuntaPreCompiled.h"
#include "CharacterSoundsSystem.h"

#include <glm/glm.hpp>

#include "FollowCameraSystem.h"
#include "Animation/Animation.h"
#include "Audio/Audio.h"
#include "Audio/AudioSystem.h"
#include "ECS/Component.h"
#include "ECS/EntityManager.h"
#include "Scene/EntityHierarchy.h"

namespace Sunta
{

namespace
{

// Moving further than this in one frame is a teleport (a respawn at the bonfire), not walking
constexpr float TeleportMeters = 2.0f;

// Rising or falling faster than this (meters per frame) is a jump or a fall: the feet don't touch the ground
constexpr float OffGroundMetersPerFrame = 0.05f;

// Slower than this (meters per second) the character stands: shifting its weight is not a step
constexpr float WalkingMetersPerSecond = 0.3f;

// A foot is in the air once its bone is this high above the lowest it gets. It lands where it stops going down, if that is
// this close to the lowest point (a run touches the ground briefly and higher than a walk). Meters for a character of
// scale 1; the lowest point creeps up slowly, so a kneel doesn't spoil it for good
constexpr float FootLiftMeters                = 0.04f;
constexpr float FootLandingMeters             = 0.06f;
constexpr float LowestFootRiseMetersPerSecond = 0.05f;

constexpr int LeftFoot  = 0;
constexpr int RightFoot = 1;

const CharacterSoundsSystem::Sound AllSounds[] =
{
	&CharacterSoundsComponent::attackSound,
	&CharacterSoundsComponent::hurtSound,
	&CharacterSoundsComponent::blockSound,
	&CharacterSoundsComponent::parrySound,
	&CharacterSoundsComponent::deathSound,
	&CharacterSoundsComponent::rollSound,
	&CharacterSoundsComponent::footstepSound,
	&CharacterSoundsComponent::shockwaveSound
};

// ========== The swoosh of an attack ==========

// How long the current animation lasts (seconds), 0 without one
float GetAnimationSeconds(EntityManager& entityManager, unsigned int entityID)
{
	const AnimatorComponent* animator  = entityManager.GetComponent<AnimatorComponent>(entityID);
	const Animation*         animation = animator ? animator->animator.GetCurrentAnimation() : nullptr;
	if (!animation || animation->GetTicksPerSecond() <= 0.0f)
		return 0.0f;

	return animation->GetDuration() / animation->GetTicksPerSecond();
}

// Once per attack, 'attackSoundLeadSeconds' before its blow lands (at once when the attack is shorter than that)
void UpdateAttackSound(EntityManager& entityManager, unsigned int entityID, CharacterSoundsComponent& sounds)
{
	const CharacterActionsComponent* actions = entityManager.GetComponent<CharacterActionsComponent>(entityID);
	if (!actions || actions->attackNumber == sounds.soundedAttackNumber)
		return;

	const float hitFrom = actions->GetHitFrom();
	if (hitFrom < 0.0f)
		return;

	const float secondsToBlow = (hitFrom - actions->actionProgress) * GetAnimationSeconds(entityManager, entityID);
	if (secondsToBlow > sounds.attackSoundLeadSeconds)
		return;

	sounds.soundedAttackNumber = actions->attackNumber;
	AudioSystem::PlayAtEntity(entityManager, entityID, sounds.attackSound, sounds.volume);
}

// ========== Footsteps ==========

// The sound of a step, and the ground trembling under a heavy one (footstepCameraShake)
void PlayFootstep(EntityManager& entityManager, unsigned int entityID, const CharacterSoundsComponent& sounds)
{
	AudioSystem::PlayAtEntity(entityManager, entityID, sounds.footstepSound, sounds.volume);

	const TransformComponent* transform = entityManager.GetComponent<TransformComponent>(entityID);
	if (sounds.footstepCameraShake > 0.0f && transform)
		FollowCameraSystem::ShakeFrom(entityManager, sounds.footstepCameraShake, transform->position, sounds.footstepShakeRadius);
}

// Walking on its own feet: alive, not rolling or attacking (the action moves the body then), not jumping, not standing
bool IsWalking(EntityManager& entityManager, unsigned int entityID, const glm::vec3& step, float deltaTime)
{
	const HealthComponent* health = entityManager.GetComponent<HealthComponent>(entityID);
	if (health && health->isDead)
		return false;

	const CharacterControllerComponent* controller = entityManager.GetComponent<CharacterControllerComponent>(entityID);
	if (controller && controller->isDrivenByAction)
		return false;

	const float meters = glm::length(glm::vec2(step.x, step.z));
	return std::abs(step.y) < OffGroundMetersPerFrame && meters <= TeleportMeters && meters >= WalkingMetersPerSecond * deltaTime;
}

// How high each foot's bone is above the character's feet (its Transform), in meters of a character of scale 1
bool TryGetFootHeights(EntityManager& entityManager, unsigned int entityID, const CharacterSoundsComponent& sounds,
	const TransformComponent& transform, float outHeights[CharacterSoundsComponent::FootCount])
{
	const std::string* bones[CharacterSoundsComponent::FootCount] = { &sounds.leftFootBone, &sounds.rightFootBone };
	const float scale = glm::max(transform.scale.y, 0.01f);

	for (int foot = LeftFoot; foot <= RightFoot; foot++)
	{
		glm::mat4 boneWorld;
		if (bones[foot]->empty() || !EntityHierarchy::TryGetBoneWorldTransform(entityManager, entityID, *bones[foot], boneWorld))
			return false;

		outHeights[foot] = (boneWorld[3].y - transform.position.y) / scale;
	}

	return true;
}

// A foot that was in the air stops going down close to the ground: that is a step
void UpdateFeet(EntityManager& entityManager, unsigned int entityID, CharacterSoundsComponent& sounds,
	const float heights[CharacterSoundsComponent::FootCount], bool isWalking, float deltaTime)
{
	if (!sounds.hasFootHeights)
	{
		for (int foot = LeftFoot; foot <= RightFoot; foot++)
		{
			sounds.lowestFootHeight[foot]   = heights[foot];
			sounds.previousFootHeight[foot] = heights[foot];
		}
		sounds.hasFootHeights = true;
	}

	for (int foot = LeftFoot; foot <= RightFoot; foot++)
	{
		float& lowest = sounds.lowestFootHeight[foot];
		lowest = glm::min(lowest + LowestFootRiseMetersPerSecond * deltaTime, heights[foot]);

		const float previous = sounds.previousFootHeight[foot];
		sounds.previousFootHeight[foot] = heights[foot];

		if (heights[foot] > lowest + FootLiftMeters)
			sounds.isFootLifted[foot] = true;

		// Going down, then not any more: the lowest point of this step (rising slowly onto the toes afterwards is not a step)
		const bool isGoingDown = heights[foot] < previous;
		const bool hasLanded   = sounds.isFootGoingDown[foot] && !isGoingDown;
		sounds.isFootGoingDown[foot] = isGoingDown;

		if (!sounds.isFootLifted[foot] || !hasLanded || previous > lowest + FootLandingMeters)
			continue;

		sounds.isFootLifted[foot] = false;
		if (isWalking)
			PlayFootstep(entityManager, entityID, sounds);
	}
}

// For a model without foot bones: a step every 'footstepMeters' walked
void CountMeters(EntityManager& entityManager, unsigned int entityID, CharacterSoundsComponent& sounds, float meters)
{
	sounds.metersSinceFootstep += meters;
	if (sounds.metersSinceFootstep < sounds.footstepMeters)
		return;

	sounds.metersSinceFootstep = 0.0f;
	PlayFootstep(entityManager, entityID, sounds);
}

void UpdateFootsteps(EntityManager& entityManager, unsigned int entityID, CharacterSoundsComponent& sounds, float deltaTime)
{
	const TransformComponent* transform = entityManager.GetComponent<TransformComponent>(entityID);
	if (!transform)
		return;

	const glm::vec3 step = transform->position - sounds.lastPosition;
	const bool      hadLastPosition = sounds.hasLastPosition;

	sounds.lastPosition    = transform->position;
	sounds.hasLastPosition = true;

	const bool isWalking = hadLastPosition && IsWalking(entityManager, entityID, step, deltaTime);

	float heights[CharacterSoundsComponent::FootCount];
	if (TryGetFootHeights(entityManager, entityID, sounds, *transform, heights))
		UpdateFeet(entityManager, entityID, sounds, heights, isWalking, deltaTime);
	else if (isWalking)
		CountMeters(entityManager, entityID, sounds, glm::length(glm::vec2(step.x, step.z)));
}

}

void CharacterSoundsSystem::PrepareForPlay(EntityManager& entityManager)
{
	for (CharacterSoundsComponent& sounds : entityManager.GetAllComponents<CharacterSoundsComponent>())
	{
		const unsigned int entityID = entityManager.GetEntityIDForComponent(sounds);
		const CharacterActionsComponent* actions = entityManager.GetComponent<CharacterActionsComponent>(entityID);

		sounds.metersSinceFootstep = 0.0f;
		sounds.hasLastPosition     = false;
		sounds.hasFootHeights      = false;
		sounds.isFootLifted[LeftFoot]  = false;
		sounds.isFootLifted[RightFoot] = false;
		sounds.soundedAttackNumber = actions ? actions->attackNumber : 0;

		for (Sound sound : AllSounds)
			Audio::Preload(sounds.*sound);
	}
}

void CharacterSoundsSystem::Update(EntityManager& entityManager, float deltaTime)
{
	for (CharacterSoundsComponent& sounds : entityManager.GetAllComponents<CharacterSoundsComponent>())
	{
		const unsigned int entityID = entityManager.GetEntityIDForComponent(sounds);

		UpdateAttackSound(entityManager, entityID, sounds);
		UpdateFootsteps(entityManager, entityID, sounds, deltaTime);
	}
}

void CharacterSoundsSystem::Play(EntityManager& entityManager, unsigned int entityID, Sound sound)
{
	if (const CharacterSoundsComponent* sounds = entityManager.GetComponent<CharacterSoundsComponent>(entityID))
		AudioSystem::PlayAtEntity(entityManager, entityID, sounds->*sound, sounds->volume);
}

}
