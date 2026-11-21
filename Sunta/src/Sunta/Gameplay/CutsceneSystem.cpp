#include "Core/SuntaPreCompiled.h"
#include "CutsceneSystem.h"
#include "HUDSystem.h"

#include <cctype>
#include <cmath>
#include <vector>

#include "CharacterActionsSystem.h"
#include "EntityName.h"
#include "GameplayMath.h"
#include "Audio/Audio.h"
#include "Core/InputManager.h"
#include "Core/KeyCodes.h"
#include "Core/Log.h"
#include "ECS/Component.h"
#include "ECS/EntityManager.h"
#include "Renderer/UIRenderer.h"
#include "Renderer/Texture.h"
#include "Renderer/VideoPlayer.h"
#include "Core/VirtualFileSystem.h"
#include "Scene/EntityHierarchy.h"
#include "Core/ResourceManager.h"
#include "Physics/CollisionShapes.h"
#include "Renderer/Model.h"

namespace Sunta
{

namespace
{

constexpr int         SkipKey          = SUNTA_KEY_E;
constexpr float       SkipAfterSeconds = 0.5f;
constexpr float       MaxStepSeconds   = 0.1f;   // a key held from the fog doesn't skip it at once
constexpr const char* SkipHint         = "[E] skip";

// The black bars: part of the screen's height each, and how long they take to slide in and out
constexpr float BarHeight      = 0.12f;
constexpr float BarFadeSeconds = 0.6f;

// The cutscene's sound fades out this long when it ends or is skipped
constexpr float SoundFadeOutSeconds = 0.4f;
// After a cutscene the bars wait this long before sliding out: the next shot of a sequence keeps them
constexpr float BarLingerSeconds = 0.9f;

// How far the bars are in (0..1), shared by every cutscene so a sequence of shots shows them once
struct Letterbox
{
	float shown          = 0.0f;
	float secondsWithout = 0.0f;
	bool  isHeld         = false;
};

Letterbox& GetLetterbox()
{
	static Letterbox letterbox;
	return letterbox;
}

constexpr float     TitleHeight = 64.0f;
const glm::vec4     TitleColor  = glm::vec4(0.92f, 0.78f, 0.52f, 1.0f);
constexpr float     HintHeight  = 18.0f;
constexpr float     PromptHeight = 30.0f;
const glm::vec4     HintColor   = glm::vec4(0.7f, 0.7f, 0.7f, 0.6f);
const glm::vec3     WorldUp     = glm::vec3(0.0f, 1.0f, 0.0f);

// Scenery between the camera and what it looks at: the camera comes in front of it (this far in front), at once,
// and goes back out slowly when the way is clear again. Not closer to what it looks at than the nearest distance
constexpr float SceneryMargin         = 0.6f;
constexpr float NearestClearDistance  = 1.2f;
constexpr float ClearingMetersPerSecond = 6.0f;
// Small things (a weapon, a thrown bottle) don't hide a shot
constexpr float SmallestHidingSize    = 1.0f;

// One video plays at a time
VideoPlayer& GetVideo()
{
	static VideoPlayer video;
	return video;
}


glm::vec3 GetWorldPosition(EntityManager& entityManager, unsigned int entityID)
{
	if (const WorldMatrixComponent* world = entityManager.GetComponent<WorldMatrixComponent>(entityID))
		return glm::vec3(world->matrix[3]);

	const TransformComponent* transform = entityManager.GetComponent<TransformComponent>(entityID);
	return transform ? transform->position : glm::vec3(0.0f);
}

// A smooth curve through the points (Catmull-Rom): at 0 the first point, at 1 the last one
glm::vec3 PointOnPath(const std::vector<glm::vec3>& points, float along)
{
	if (points.size() == 1)
		return points.front();

	const float scaled  = glm::clamp(along, 0.0f, 1.0f) * (float)(points.size() - 1);
	const int   segment = glm::min((int)scaled, (int)points.size() - 2);
	const float t       = scaled - (float)segment;

	const glm::vec3& p1 = points[segment];
	const glm::vec3& p2 = points[segment + 1];
	const glm::vec3& p0 = segment > 0 ? points[segment - 1] : p1;
	const glm::vec3& p3 = segment + 2 < (int)points.size() ? points[segment + 2] : p2;

	const float t2 = t * t;
	const float t3 = t2 * t;
	return 0.5f * ((2.0f * p1) + (-p0 + p2) * t + (2.0f * p0 - 5.0f * p1 + 4.0f * p2 - p3) * t2 + (-p0 + 3.0f * p1 - 3.0f * p2 + p3) * t3);
}

// The rotation of an entity that looks along 'forward' (its -Z axis) with +Y up (as the follow camera does)
glm::quat MakeRotationLookingAlong(const glm::vec3& forward)
{
	const glm::vec3 right = glm::normalize(glm::cross(forward, WorldUp));
	const glm::vec3 up    = glm::cross(right, forward);
	return glm::quat_cast(glm::mat3(right, up, -forward));
}

TransformComponent* FindActiveCamera(EntityManager& entityManager)
{
	for (CameraComponent& camera : entityManager.GetAllComponents<CameraComponent>())
	{
		if (camera.isActive)
			return entityManager.GetComponent<TransformComponent>(entityManager.GetEntityIDForComponent(camera));
	}
	return nullptr;
}

// Nobody fights while it plays: everyone stops where they stand, the player's keys are switched off
void HoldEveryone(EntityManager& entityManager, bool isHeld)
{
	for (CharacterControllerComponent& controller : entityManager.GetAllComponents<CharacterControllerComponent>())
	{
		controller.moveDirection    = glm::vec3(0.0f);
		controller.wantsToRun       = false;
		controller.isHeldByCutscene = isHeld && controller.isPlayerControlled;
	}
}

bool IsProjectileBody(EntityManager& entityManager, unsigned int entityID)
{
	unsigned int parentID = 0;
	return EntityHierarchy::TryGetParent(entityManager, entityID, parentID)
		&& entityManager.GetComponent<ProjectileComponent>(parentID) != nullptr;
}

// How far from 'target' towards 'camera' the view is free of scenery: the closest triangle of a visible model without
// bones (ruins, rubble, walls) on the line between them. A model the target is inside (a temple whose heart the camera
// looks at: the line leaves it instead of entering it) doesn't count, nor small things and the bodies of projectiles
float GetClearDistance(EntityManager& entityManager, const glm::vec3& target, const glm::vec3& camera)
{
	const glm::vec3 toCamera = camera - target;
	const float wanted = glm::length(toCamera);
	if (wanted < 0.001f)
		return wanted;

	Ray ray;
	ray.origin    = target;
	ray.direction = toCamera / wanted;

	float clear = wanted;
	for (ModelComponent& model : entityManager.GetAllComponents<ModelComponent>())
	{
		if (!model.isVisible || !model.modelData || !model.modelData->model || model.modelData->model->HasBones())
			continue;

		const unsigned int entityID = entityManager.GetEntityIDForComponent(model);
		const WorldMatrixComponent* world = entityManager.GetComponent<WorldMatrixComponent>(entityID);
		if (!world)
			continue;

		const Model& shape = *model.modelData->model;
		const glm::vec3 boundsMin = shape.GetLocalBoundsMin();
		const glm::vec3 boundsMax = shape.GetLocalBoundsMax();
		const OBB box = MakeWorldOBB(world->matrix, (boundsMin + boundsMax) * 0.5f, (boundsMax - boundsMin) * 0.5f);

		if (glm::length(box.halfExtents) * 2.0f < SmallestHidingSize || IsProjectileBody(entityManager, entityID))
			continue;

		float boxDistance = 0.0f;
		if (!RayIntersectsOBB(ray, box, boxDistance) || boxDistance >= clear)
			continue;

		// The triangles, in the model's own space (the direction keeps its length: the distance comes out in world units)
		const glm::mat4 toLocal = glm::inverse(world->matrix);
		const glm::vec3 localOrigin = glm::vec3(toLocal * glm::vec4(target, 1.0f));
		const glm::vec3 localDirection = glm::vec3(toLocal * glm::vec4(ray.direction, 0.0f));

		float hitDistance = 0.0f;
		bool  isEntering  = false;
		if (shape.RaycastTriangles(localOrigin, localDirection, hitDistance, isEntering) && isEntering && hitDistance < clear)
			clear = hitDistance;
	}
	return clear;
}

void PlaceCamera(EntityManager& entityManager, unsigned int cutsceneID, CutsceneComponent& cutscene, float deltaTime)
{
	TransformComponent* camera = FindActiveCamera(entityManager);
	if (!camera)
		return;

	std::vector<glm::vec3> points;
	for (unsigned int childID : EntityHierarchy::GetChildren(entityManager, cutsceneID))
		points.push_back(GetWorldPosition(entityManager, childID));
	if (points.empty())
		return;

	// Eased: the camera starts and stops gently
	const float linear = glm::clamp(cutscene.seconds / cutscene.durationSeconds, 0.0f, 1.0f);
	const float along  = linear * linear * (3.0f - 2.0f * linear);
	glm::vec3 position = PointOnPath(points, along);

	glm::vec3 lookDirection = PointOnPath(points, glm::min(along + 0.02f, 1.0f)) - position;
	unsigned int targetID = 0;
	if (TryFindEntityByName(entityManager, cutscene.lookAtEntity, targetID))
	{
		const glm::vec3 target = GetWorldPosition(entityManager, targetID) + WorldUp * cutscene.lookAtHeight;
		const float wanted = glm::length(position - target);

		// Scenery in the way: the camera comes in front of it at once, and back out gently
		const float clear = glm::max(GetClearDistance(entityManager, target, position) - SceneryMargin, NearestClearDistance);
		if (cutscene.clearDistance < 0.0f || clear < cutscene.clearDistance)
			cutscene.clearDistance = clear;
		else
			cutscene.clearDistance = glm::min(cutscene.clearDistance + ClearingMetersPerSecond * deltaTime, clear);

		if (wanted > cutscene.clearDistance && wanted > 0.001f)
			position = target + (position - target) * (cutscene.clearDistance / wanted);

		lookDirection = target - position;
	}

	if (glm::length(lookDirection) < 0.001f || std::abs(glm::normalize(lookDirection).y) > 0.999f)
		return;

	glm::quat rotation = MakeRotationLookingAlong(glm::normalize(lookDirection));

	// Flying in from where the camera was (eased), instead of a cut
	if (cutscene.canBlend && cutscene.seconds < cutscene.blendInSeconds)
	{
		const float linearBlend = glm::clamp(cutscene.seconds / cutscene.blendInSeconds, 0.0f, 1.0f);
		const float blend = linearBlend * linearBlend * (3.0f - 2.0f * linearBlend);
		position = glm::mix(cutscene.blendFromPosition, position, blend);
		rotation = glm::slerp(cutscene.blendFromRotation, rotation, blend);
	}

	camera->position           = position;
	camera->rotationQuaternion = rotation;
	camera->SyncEulerFromQuaternion();
	camera->isDirty = true;
}

void Finish(EntityManager& entityManager, CutsceneComponent& cutscene)
{
	if (cutscene.isVideo)
		GetVideo().Close();

	SUNTA_ENGINE_LOG_INFO("Cutscene: finished after {0} s", cutscene.seconds);
	cutscene.isPlaying = false;
	HoldEveryone(entityManager, false);

	if (cutscene.soundVoice != Audio::NoVoice)
		Audio::Stop(cutscene.soundVoice, SoundFadeOutSeconds);
	cutscene.soundVoice = Audio::NoVoice;

	// The game comes back out of the black
	if (cutscene.fadeSeconds > 0.0f)
		HUDSystem::FlashScreen(glm::vec3(0.0f), cutscene.fadeSeconds);
}

}

void CutsceneSystem::Play(EntityManager& entityManager, unsigned int cutsceneEntityID)
{
	CutsceneComponent* cutscene = entityManager.GetComponent<CutsceneComponent>(cutsceneEntityID);
	if (!cutscene || cutscene->hasPlayed)
		return;


	SUNTA_ENGINE_LOG_INFO("Cutscene: '{0}' starts", GetEntityName(entityManager, cutsceneEntityID));

	// A shot started while another one plays is a cut: it takes the camera over at once (no frame of the game
	// between two shots of one scene). A video keeps playing under it
	for (CutsceneComponent& other : entityManager.GetAllComponents<CutsceneComponent>())
	{
		if (&other == cutscene || !other.isPlaying || other.isVideo)
			continue;
		if (other.soundVoice != Audio::NoVoice)
			Audio::Stop(other.soundVoice, SoundFadeOutSeconds);
		other.soundVoice = Audio::NoVoice;
		other.isPlaying  = false;
	}

	cutscene->isPlaying           = true;
	cutscene->hasPlayed           = true;
	cutscene->hasStartedAnimation = false;
	cutscene->seconds             = 0.0f;
	cutscene->clearDistance       = -1.0f;
	cutscene->wasAnswered         = false;
	cutscene->fadeToBlackLeft     = cutscene->fadeSeconds;
	cutscene->hasStartedSound     = false;
	cutscene->pressProgress       = 0.0f;

	// Where the camera is now: a cutscene with 'blendInSeconds' flies from here
	const TransformComponent* camera = FindActiveCamera(entityManager);
	cutscene->canBlend = camera != nullptr && cutscene->blendInSeconds > 0.0f;
	if (camera)
	{
		cutscene->blendFromPosition = camera->position;
		cutscene->blendFromRotation = camera->rotationQuaternion;
	}

	// A video that can't be opened leaves the camera cutscene (the error is in the log)
	cutscene->isVideo = !cutscene->videoFile.empty() && GetVideo().Open(VirtualFileSystem::Resolve(cutscene->videoFile));
	HoldEveryone(entityManager, true);
}

bool CutsceneSystem::PlayByName(EntityManager& entityManager, const std::string& name)
{
	unsigned int entityID = 0;
	if (!TryFindEntityByName(entityManager, name, entityID) || !entityManager.GetComponent<CutsceneComponent>(entityID))
		return false;

	Play(entityManager, entityID);
	return true;
}

bool CutsceneSystem::IsPlaying(EntityManager& entityManager)
{
	for (const CutsceneComponent& cutscene : entityManager.GetAllComponents<CutsceneComponent>())
	{
		if (cutscene.isPlaying)
			return true;
	}
	return false;
}

void CutsceneSystem::HoldLetterbox(bool isHeld)
{
	GetLetterbox().isHeld = isHeld;
}

void CutsceneSystem::Update(EntityManager& entityManager, float deltaTime)
{
	// The bars: in while a shot (not a video) plays or a script holds them, out a little after the last shot
	Letterbox& letterbox = GetLetterbox();
	bool isShotPlaying = false;
	for (const CutsceneComponent& cutscene : entityManager.GetAllComponents<CutsceneComponent>())
		isShotPlaying = isShotPlaying || (cutscene.isPlaying && !cutscene.isVideo);

	const float step = glm::min(deltaTime, MaxStepSeconds) / BarFadeSeconds;
	letterbox.secondsWithout = isShotPlaying || letterbox.isHeld ? 0.0f : letterbox.secondsWithout + deltaTime;
	if (letterbox.secondsWithout <= 0.0f)
		letterbox.shown = glm::min(letterbox.shown + step, 1.0f);
	else if (letterbox.secondsWithout > BarLingerSeconds)
		letterbox.shown = glm::max(letterbox.shown - step, 0.0f);

	for (CutsceneComponent& cutscene : entityManager.GetAllComponents<CutsceneComponent>())
	{
		if (!cutscene.isPlaying)
			continue;

		// First the game fades to black (everyone already waits): the cutscene itself hasn't started
		if (cutscene.fadeToBlackLeft > 0.0f)
		{
			cutscene.fadeToBlackLeft -= glm::min(deltaTime, MaxStepSeconds);
			continue;
		}

		if (!cutscene.hasStartedSound)
		{
			cutscene.hasStartedSound = true;
			if (!cutscene.sound.empty())
			{
				Audio::PlayOptions options;
				options.isStreamed = true;
				cutscene.soundVoice = Audio::Play(cutscene.sound, options);
			}
		}

		const unsigned int cutsceneID = entityManager.GetEntityIDForComponent(cutscene);
		// A long frame (loading something) must not skip the cutscene
		cutscene.seconds += glm::min(deltaTime, MaxStepSeconds);

		// A moment the player answers: it holds at its end until the key (letters and digits are their characters)
		if (!cutscene.isVideo && !cutscene.waitForKey.empty())
		{
			const int key = std::toupper(static_cast<unsigned char>(cutscene.waitForKey.front()));
			const bool isPressed = InputManager::IsKeyDown(key);

			// Many presses: each one adds, the tempo of 'pressWindowSeconds' is what keeps the progress from draining
			if (cutscene.waitPresses > 1)
			{
				const float window = glm::max(cutscene.pressWindowSeconds, 0.05f);
				cutscene.pressProgress = glm::max(cutscene.pressProgress - glm::min(deltaTime, MaxStepSeconds) / window, 0.0f);
				if (isPressed)
					cutscene.pressProgress += 1.0f;
			}
			else if (isPressed)
				cutscene.pressProgress = 1.0f;

			if (cutscene.pressProgress >= static_cast<float>(cutscene.waitPresses))
			{
				cutscene.wasAnswered = true;
				Finish(entityManager, cutscene);
				continue;
			}
			cutscene.seconds = glm::min(cutscene.seconds, cutscene.durationSeconds - 0.001f);
		}

		const bool isSkipped = cutscene.canBeSkipped && cutscene.seconds > SkipAfterSeconds && InputManager::IsKeyDown(SkipKey);

		if (cutscene.isVideo)
		{
			GetVideo().Update(deltaTime);
			if (isSkipped || GetVideo().IsFinished())
				Finish(entityManager, cutscene);
			continue;
		}

		if (!cutscene.hasStartedAnimation && cutscene.seconds >= cutscene.animationAtSeconds)
		{
			cutscene.hasStartedAnimation = true;

			unsigned int actorID = 0;
			if (TryFindEntityByName(entityManager, cutscene.animationEntity, actorID) && !cutscene.animationName.empty())
				CharacterActionsSystem::PlaySpecial(entityManager, actorID, cutscene.animationName);
		}

		PlaceCamera(entityManager, cutsceneID, cutscene, deltaTime);

		if (isSkipped || cutscene.seconds >= cutscene.durationSeconds)
			Finish(entityManager, cutscene);
	}
}

void CutsceneSystem::DrawOverlay(EntityManager& entityManager, const glm::vec2& screenSize, const glm::vec2& areaTopLeft, const glm::vec2& areaSize)
{
	// The bars (see Letterbox), eased in and out
	const float shown = GetLetterbox().shown;
	const float bars  = shown * shown * (3.0f - 2.0f * shown) * BarHeight * areaSize.y;
	if (bars > 0.0f)
	{
		UIRenderer::Begin(screenSize);
		UIRenderer::DrawRectangle(areaTopLeft, glm::vec2(areaSize.x, bars), glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
		UIRenderer::DrawRectangle(areaTopLeft + glm::vec2(0.0f, areaSize.y - bars), glm::vec2(areaSize.x, bars), glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
		UIRenderer::End();
	}

	for (const CutsceneComponent& cutscene : entityManager.GetAllComponents<CutsceneComponent>())
	{
		if (!cutscene.isPlaying || cutscene.fadeToBlackLeft > 0.0f)
			continue;

		// A video fills the game's area as much as it can without stretching, black around it
		if (cutscene.isVideo)
		{
			UIRenderer::Begin(screenSize);
			UIRenderer::DrawRectangle(areaTopLeft, areaSize, glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));

			const VideoPlayer& video = GetVideo();
			if (const Texture* frame = video.GetTexture())
			{
				const float scale = glm::min(areaSize.x / (float)video.GetWidth(), areaSize.y / (float)video.GetHeight());
				const glm::vec2 size = glm::vec2((float)video.GetWidth(), (float)video.GetHeight()) * scale;
				UIRenderer::DrawImage(areaTopLeft + (areaSize - size) * 0.5f, size, *frame);
			}

			if (cutscene.canBeSkipped)
				UIRenderer::DrawTextAt(SkipHint, areaTopLeft + glm::vec2(areaSize.x - 110.0f, areaSize.y - 30.0f), HintHeight, HintColor);

			UIRenderer::End();
			continue;
		}

		// The title fades out with the end of its shot
		const float out  = glm::clamp((cutscene.durationSeconds - cutscene.seconds) / BarFadeSeconds, 0.0f, 1.0f);
		const glm::vec2 bottomLeft = areaTopLeft + glm::vec2(0.0f, areaSize.y - bars);

		UIRenderer::Begin(screenSize);

		const float titleStart = cutscene.durationSeconds - cutscene.titleSeconds;
		if (!cutscene.title.empty() && cutscene.seconds >= titleStart)
		{
			const float fade = glm::clamp((cutscene.seconds - titleStart) / 1.0f, 0.0f, 1.0f) * out;
			UIRenderer::DrawTextCentered(cutscene.title, areaTopLeft + glm::vec2(areaSize.x * 0.5f, areaSize.y * 0.78f), TitleHeight,
				glm::vec4(glm::vec3(TitleColor), fade));
		}

		if (cutscene.canBeSkipped)
			UIRenderer::DrawTextAt(SkipHint, bottomLeft + glm::vec2(areaSize.x - 110.0f, 8.0f), HintHeight, HintColor);

		// What the player is asked to press, pulsing over the lower bar once the camera has come to rest
		if (!cutscene.waitForKey.empty() && !cutscene.waitPrompt.empty())
		{
			const float shownFor = cutscene.seconds - glm::max(cutscene.durationSeconds - 1.0f, 0.0f);
			const float pulse    = 0.75f + 0.25f * std::sin(cutscene.seconds * 5.0f);
			const float fadeIn   = glm::clamp(shownFor / 0.5f, 0.0f, 1.0f);
			UIRenderer::DrawTextCentered(cutscene.waitPrompt, areaTopLeft + glm::vec2(areaSize.x * 0.5f, areaSize.y * 0.86f),
				PromptHeight, glm::vec4(glm::vec3(TitleColor), fadeIn * pulse));

			// How far the presses got: a bar under the prompt
			if (cutscene.waitPresses > 1)
			{
				const float part = glm::clamp(cutscene.pressProgress / static_cast<float>(cutscene.waitPresses), 0.0f, 1.0f);
				const glm::vec2 barSize = glm::vec2(areaSize.y * 0.36f, glm::max(areaSize.y * 0.012f, 5.0f));
				const glm::vec2 barTopLeft = areaTopLeft + glm::vec2((areaSize.x - barSize.x) * 0.5f, areaSize.y * 0.905f);
				UIRenderer::DrawRectangle(barTopLeft - glm::vec2(2.0f), barSize + glm::vec2(4.0f), glm::vec4(0.0f, 0.0f, 0.0f, 0.7f * fadeIn));
				UIRenderer::DrawRectangle(barTopLeft, glm::vec2(barSize.x * part, barSize.y), glm::vec4(glm::vec3(TitleColor), fadeIn));
			}
		}

		UIRenderer::End();
	}

	// The fades (see fadeSeconds): the game into black before the cutscene, the cutscene out of it
	for (const CutsceneComponent& cutscene : entityManager.GetAllComponents<CutsceneComponent>())
	{
		if (!cutscene.isPlaying || cutscene.fadeSeconds <= 0.0f)
			continue;

		const float black = cutscene.fadeToBlackLeft > 0.0f ? 1.0f - cutscene.fadeToBlackLeft / cutscene.fadeSeconds
		                                                    : 1.0f - cutscene.seconds / cutscene.fadeSeconds;
		if (black <= 0.0f)
			continue;

		UIRenderer::Begin(screenSize);
		UIRenderer::DrawRectangle(areaTopLeft, areaSize, glm::vec4(0.0f, 0.0f, 0.0f, glm::clamp(black, 0.0f, 1.0f)));
		UIRenderer::End();
	}
}

void CutsceneSystem::PrepareForPlay(EntityManager& entityManager)
{
	GetLetterbox() = Letterbox();

	for (CutsceneComponent& cutscene : entityManager.GetAllComponents<CutsceneComponent>())
	{
		cutscene.isPlaying           = false;
		cutscene.hasPlayed           = false;
		cutscene.hasStartedAnimation = false;
		cutscene.seconds             = 0.0f;
	}
}

}
