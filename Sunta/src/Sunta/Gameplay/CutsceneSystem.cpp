#include "Core/SuntaPreCompiled.h"
#include "CutsceneSystem.h"

#include <vector>

#include "CharacterActionsSystem.h"
#include "EntityName.h"
#include "GameplayMath.h"
#include "Core/InputManager.h"
#include "Core/KeyCodes.h"
#include "Core/Log.h"
#include "ECS/Component.h"
#include "ECS/EntityManager.h"
#include "Renderer/UIRenderer.h"
#include "Scene/EntityHierarchy.h"

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

constexpr float     TitleHeight = 64.0f;
const glm::vec4     TitleColor  = glm::vec4(0.92f, 0.78f, 0.52f, 1.0f);
constexpr float     HintHeight  = 18.0f;
const glm::vec4     HintColor   = glm::vec4(0.7f, 0.7f, 0.7f, 0.6f);
const glm::vec3     WorldUp     = glm::vec3(0.0f, 1.0f, 0.0f);

bool TryFindByName(EntityManager& entityManager, const std::string& name, unsigned int& outEntityID)
{
	if (name.empty())
		return false;

	for (TagComponent& tag : entityManager.GetAllComponents<TagComponent>())
	{
		if (tag.name == name)
		{
			outEntityID = entityManager.GetEntityIDForComponent(tag);
			return true;
		}
	}
	return false;
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

void PlaceCamera(EntityManager& entityManager, unsigned int cutsceneID, const CutsceneComponent& cutscene)
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
	const glm::vec3 position = PointOnPath(points, along);

	glm::vec3 lookDirection = PointOnPath(points, glm::min(along + 0.02f, 1.0f)) - position;
	unsigned int targetID = 0;
	if (TryFindByName(entityManager, cutscene.lookAtEntity, targetID))
		lookDirection = GetWorldPosition(entityManager, targetID) + WorldUp * cutscene.lookAtHeight - position;

	if (glm::length(lookDirection) < 0.001f || std::abs(glm::normalize(lookDirection).y) > 0.999f)
		return;

	camera->position           = position;
	camera->rotationQuaternion = MakeRotationLookingAlong(glm::normalize(lookDirection));
	camera->SyncEulerFromQuaternion();
	camera->isDirty = true;
}

void Finish(EntityManager& entityManager, CutsceneComponent& cutscene)
{
	SUNTA_ENGINE_LOG_INFO("Cutscene: finished after {:.1f} s", cutscene.seconds);
	cutscene.isPlaying = false;
	HoldEveryone(entityManager, false);
}

}

void CutsceneSystem::Play(EntityManager& entityManager, unsigned int cutsceneEntityID)
{
	CutsceneComponent* cutscene = entityManager.GetComponent<CutsceneComponent>(cutsceneEntityID);
	if (!cutscene || cutscene->hasPlayed)
		return;

	if (!cutscene->videoFile.empty())
		SUNTA_ENGINE_LOG_WARNING("Cutscene: '{0}' is a video, the engine can't play videos yet, the camera cutscene plays instead", cutscene->videoFile);

	SUNTA_ENGINE_LOG_INFO("Cutscene: '{0}' starts", GetEntityName(entityManager, cutsceneEntityID));
	cutscene->isPlaying           = true;
	cutscene->hasPlayed           = true;
	cutscene->hasStartedAnimation = false;
	cutscene->seconds             = 0.0f;
	HoldEveryone(entityManager, true);
}

bool CutsceneSystem::PlayByName(EntityManager& entityManager, const std::string& name)
{
	unsigned int entityID = 0;
	if (!TryFindByName(entityManager, name, entityID) || !entityManager.GetComponent<CutsceneComponent>(entityID))
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

void CutsceneSystem::Update(EntityManager& entityManager, float deltaTime)
{
	for (CutsceneComponent& cutscene : entityManager.GetAllComponents<CutsceneComponent>())
	{
		if (!cutscene.isPlaying)
			continue;

		const unsigned int cutsceneID = entityManager.GetEntityIDForComponent(cutscene);
		// A long frame (loading something) must not skip the cutscene
		cutscene.seconds += glm::min(deltaTime, MaxStepSeconds);

		if (!cutscene.hasStartedAnimation && cutscene.seconds >= cutscene.animationAtSeconds)
		{
			cutscene.hasStartedAnimation = true;

			unsigned int actorID = 0;
			if (TryFindByName(entityManager, cutscene.animationEntity, actorID) && !cutscene.animationName.empty())
				CharacterActionsSystem::PlaySpecial(entityManager, actorID, cutscene.animationName);
		}

		PlaceCamera(entityManager, cutsceneID, cutscene);

		const bool isSkipped = cutscene.canBeSkipped && cutscene.seconds > SkipAfterSeconds && InputManager::IsKeyDown(SkipKey);
		if (isSkipped || cutscene.seconds >= cutscene.durationSeconds)
			Finish(entityManager, cutscene);
	}
}

void CutsceneSystem::DrawOverlay(EntityManager& entityManager, const glm::vec2& screenSize, const glm::vec2& areaTopLeft, const glm::vec2& areaSize)
{
	for (const CutsceneComponent& cutscene : entityManager.GetAllComponents<CutsceneComponent>())
	{
		if (!cutscene.isPlaying)
			continue;

		// The bars slide in at the start and out at the end
		const float in   = glm::clamp(cutscene.seconds / BarFadeSeconds, 0.0f, 1.0f);
		const float out  = glm::clamp((cutscene.durationSeconds - cutscene.seconds) / BarFadeSeconds, 0.0f, 1.0f);
		const float bars = glm::min(in, out) * BarHeight * areaSize.y;
		const glm::vec2 bottomLeft = areaTopLeft + glm::vec2(0.0f, areaSize.y - bars);

		UIRenderer::Begin(screenSize);
		UIRenderer::DrawRectangle(areaTopLeft, glm::vec2(areaSize.x, bars), glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
		UIRenderer::DrawRectangle(bottomLeft, glm::vec2(areaSize.x, bars), glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));

		const float titleStart = cutscene.durationSeconds - cutscene.titleSeconds;
		if (!cutscene.title.empty() && cutscene.seconds >= titleStart)
		{
			const float fade = glm::clamp((cutscene.seconds - titleStart) / 1.0f, 0.0f, 1.0f) * out;
			UIRenderer::DrawTextCentered(cutscene.title, areaTopLeft + glm::vec2(areaSize.x * 0.5f, areaSize.y * 0.78f), TitleHeight,
				glm::vec4(glm::vec3(TitleColor), fade));
		}

		if (cutscene.canBeSkipped)
			UIRenderer::DrawTextAt(SkipHint, bottomLeft + glm::vec2(areaSize.x - 110.0f, 8.0f), HintHeight, HintColor);

		UIRenderer::End();
	}
}

void CutsceneSystem::PrepareForPlay(EntityManager& entityManager)
{
	for (CutsceneComponent& cutscene : entityManager.GetAllComponents<CutsceneComponent>())
	{
		cutscene.isPlaying           = false;
		cutscene.hasPlayed           = false;
		cutscene.hasStartedAnimation = false;
		cutscene.seconds             = 0.0f;
	}
}

}
