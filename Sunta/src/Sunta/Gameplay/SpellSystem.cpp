#include "Core/SuntaPreCompiled.h"
#include "SpellSystem.h"

#include <algorithm>
#include <cmath>
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/quaternion.hpp>

#include "CharacterActionsSystem.h"
#include "CharacterControllerSystem.h"
#include "CombatSystem.h"
#include "CutsceneSystem.h"
#include "EntityName.h"
#include "FollowCameraSystem.h"
#include "GameplayMath.h"
#include "LockOnSystem.h"
#include "SightBlockers.h"
#include "Audio/AudioSystem.h"
#include "Core/InputManager.h"
#include "Core/Log.h"
#include "ECS/Component.h"
#include "ECS/EntityManager.h"
#include "Effects/ParticleSystem.h"
#include "Physics/Raycast.h"
#include "Scene/EntityHierarchy.h"

namespace Sunta
{

namespace
{

using namespace GameplayMath;

// ========== Numbers used below ==========

// A character is aimed at this high above its feet when it has no Lock On Target
constexpr float DefaultAimHeight = 1.2f;

// A burst (a projectile with a fuse) hurts only targets within this height of it (not someone on a wall above)
constexpr float BurstHeight = 2.5f;

// The camera feels a burst up to this many of its radii away
constexpr float BurstShakeReach = 4.0f;

// Without a lock the player's spell goes to the closest enemy this near (meters) and this much ahead (the cosine of
// the angle to each side of where the player faces)
constexpr float AutoAimDistance  = 35.0f;
constexpr float AutoAimMinCosine = 0.5f;

// A projectile counts as coming at a character when it flies this straight at it (the cosine of the angle)
constexpr float IncomingMinCosine = 0.9f;

// Projectiles appearing under the target stand this many of their radii apart (the ring around the first one)
constexpr float FeetRingSpacing = 2.2f;

// ========== Projectiles in the air ==========

struct LiveProjectile
{
	unsigned int number     = 0; // its own, to tell it from the others
	unsigned int templateID = 0; // the entity with the Projectile component
	unsigned int casterID   = 0;
	unsigned int targetID   = 0;
	bool         hasTarget  = false;
	bool         isPlayerSide = false;

	glm::vec3 position  = glm::vec3(0.0f);
	glm::vec3 direction = glm::vec3(0.0f, 0.0f, -1.0f);

	// Where on the target it aims, above its feet (the chest, not the toes)
	glm::vec3 aimOffset = glm::vec3(0.0f, DefaultAimHeight, 0.0f);

	float age                 = 0.0f;
	float unbornTrailParticles = 0.0f;

	// The body it borrowed: a child of the template with a Model (see ProjectileComponent)
	unsigned int bodyID    = 0;
	bool         hasBody   = false;
	glm::vec3    bodyScale = glm::vec3(1.0f); // as the scene has it (it grows up to this while the projectile forms)
	glm::vec3    bodyRotation = glm::vec3(0.0f);

	// It left the place where it formed (see ProjectileComponent::launchDelaySeconds)
	bool hasLaunched = false;

	// Rolled through: it flies on, but doesn't hurt the same character again
	std::vector<unsigned int> passedThrough;
};

std::vector<LiveProjectile>& GetLiveProjectiles()
{
	static std::vector<LiveProjectile> projectiles;
	return projectiles;
}

// ========== The bodies of the projectiles ==========

// The front of a model is its +Z, a direction with no yaw looks along -Z
constexpr float BodyYawOffsetDegrees = 180.0f;

// A body is anything that can be seen: a Model, a Liquid Orb, a Glow Sprite
bool IsBody(EntityManager& entityManager, unsigned int entityID)
{
	return entityManager.GetComponent<TransformComponent>(entityID)
		&& (entityManager.GetComponent<ModelComponent>(entityID) || entityManager.GetComponent<LiquidOrbComponent>(entityID)
			|| entityManager.GetComponent<GlowSpriteComponent>(entityID));
}

void ShowBody(EntityManager& entityManager, unsigned int bodyID, bool isShown)
{
	if (ModelComponent* model = entityManager.GetComponent<ModelComponent>(bodyID))
		model->isVisible = isShown;
	if (LiquidOrbComponent* orb = entityManager.GetComponent<LiquidOrbComponent>(bodyID))
		orb->isVisible = isShown;
	if (GlowSpriteComponent* sprite = entityManager.GetComponent<GlowSpriteComponent>(bodyID))
		sprite->isVisible = isShown;
}

// A child of the template with a Model that no projectile in the air carries now
bool TryBorrowBody(EntityManager& entityManager, unsigned int templateID, unsigned int& outBodyID)
{
	for (unsigned int childID : EntityHierarchy::GetChildren(entityManager, templateID))
	{
		if (!IsBody(entityManager, childID))
			continue;

		const std::vector<LiveProjectile>& projectiles = GetLiveProjectiles();
		const bool isTaken = std::any_of(projectiles.begin(), projectiles.end(),
			[childID](const LiveProjectile& projectile) { return projectile.hasBody && projectile.bodyID == childID; });

		if (!isTaken)
		{
			outBodyID = childID;
			return true;
		}
	}

	return false;
}

// The body goes where the projectile is, its front along the flight, tumbling as it goes
void CarryBody(EntityManager& entityManager, const ProjectileComponent& settings, const LiveProjectile& projectile)
{
	TransformComponent* body = projectile.hasBody ? entityManager.GetComponent<TransformComponent>(projectile.bodyID) : nullptr;
	if (!body)
		return;

	// The body is a child of the template: its place is measured from there
	const TransformComponent* origin = entityManager.GetComponent<TransformComponent>(projectile.templateID);

	// While it forms it grows from nothing to its size
	const float formTime = settings.formSeconds > 0.0f ? glm::min(settings.formSeconds, settings.launchDelaySeconds) : settings.launchDelaySeconds;
	const float formed   = formTime > 0.0f ? glm::clamp(projectile.age / formTime, 0.0f, 1.0f) : 1.0f;

	body->scale    = projectile.bodyScale * glm::max(formed * formed * (3.0f - 2.0f * formed), 0.001f);
	body->position = projectile.position - (origin ? origin->position : glm::vec3(0.0f));
	// Its front follows the flight, also up and down (something falling from the sky comes nose first)
	if (!settings.bodyFacesFlight)
	{
		body->rotation = projectile.bodyRotation;
		body->SyncQuaternionFromEuler();
		body->isDirty = true;
		return;
	}

	if (settings.spinsAroundFlight)
	{
		// Faced along the flight, then turned around that line (its own front axis)
		const glm::quat facing = glm::quat(glm::radians(glm::vec3(GetPitchOf(projectile.direction),
			WrapDegrees(GetYawToFace(projectile.direction) + BodyYawOffsetDegrees), 0.0f)));
		const glm::quat spin = glm::angleAxis(glm::radians(WrapDegrees(projectile.age * settings.spinDegreesPerSecond)),
			glm::vec3(0.0f, 0.0f, 1.0f));
		body->rotationQuaternion = facing * spin;
		body->SyncEulerFromQuaternion();
		body->isDirty = true;
		return;
	}

	body->rotation = glm::vec3(WrapDegrees(projectile.age * settings.spinDegreesPerSecond + GetPitchOf(projectile.direction)),
		WrapDegrees(GetYawToFace(projectile.direction) + BodyYawOffsetDegrees), 0.0f);
	body->SyncQuaternionFromEuler();
	body->isDirty = true;
}

void ReturnBody(EntityManager& entityManager, const LiveProjectile& projectile)
{
	if (!projectile.hasBody)
		return;

	ShowBody(entityManager, projectile.bodyID, false);

	// Back to the size the scene gave it, for the next projectile that borrows it
	if (TransformComponent* body = entityManager.GetComponent<TransformComponent>(projectile.bodyID))
	{
		body->scale    = projectile.bodyScale;
		body->rotation = projectile.bodyRotation;
		body->SyncQuaternionFromEuler();
		body->isDirty  = true;
	}
}

// Every body waits unseen until a projectile borrows it
void HideAllBodies(EntityManager& entityManager)
{
	for (ProjectileComponent& settings : entityManager.GetAllComponents<ProjectileComponent>())
	{
		for (unsigned int childID : EntityHierarchy::GetChildren(entityManager, entityManager.GetEntityIDForComponent(settings)))
			ShowBody(entityManager, childID, false);
	}
}

// ========== Sides and targets ==========

bool IsOnPlayersSide(EntityManager& entityManager, unsigned int entityID)
{
	const CharacterControllerComponent* controller = entityManager.GetComponent<CharacterControllerComponent>(entityID);
	return controller && controller->isPlayerControlled;
}

// The closest living enemy in front of the player (a spell thrown without a lock still looks for someone)
bool TryFindEnemyAhead(EntityManager& entityManager, unsigned int casterID, unsigned int& outTargetID)
{
	const TransformComponent*        caster  = entityManager.GetComponent<TransformComponent>(casterID);
	const CharacterActionsComponent* actions = entityManager.GetComponent<CharacterActionsComponent>(casterID);
	if (!caster || !actions)
		return false;

	float closest = AutoAimDistance;
	bool  isFound = false;

	for (EnemyComponent& enemy : entityManager.GetAllComponents<EnemyComponent>())
	{
		const unsigned int enemyID = entityManager.GetEntityIDForComponent(enemy);
		const HealthComponent*    health    = entityManager.GetComponent<HealthComponent>(enemyID);
		const TransformComponent* transform = entityManager.GetComponent<TransformComponent>(enemyID);
		if (enemy.isDormant || !health || health->isDead || !transform)
			continue;

		glm::vec3 toEnemy = transform->position - caster->position;
		toEnemy.y = 0.0f;
		const float distance = glm::length(toEnemy);
		if (distance < SmallestUsableLength || distance >= closest || glm::dot(toEnemy / distance, actions->actionDirection) < AutoAimMinCosine)
			continue;

		closest     = distance;
		outTargetID = enemyID;
		isFound     = true;
	}

	return isFound;
}

// The one the caster fights: its locked-on enemy, else (for an enemy) the player, (for the player) the closest enemy ahead
bool TryFindTarget(EntityManager& entityManager, unsigned int casterID, unsigned int& outTargetID)
{
	const LockOnComponent* lockOn = entityManager.GetComponent<LockOnComponent>(casterID);
	if (lockOn && lockOn->hasTarget)
	{
		outTargetID = lockOn->targetEntityID;
		return true;
	}

	if (IsOnPlayersSide(entityManager, casterID))
		return TryFindEnemyAhead(entityManager, casterID, outTargetID);

	return CharacterControllerSystem::TryFindPlayer(entityManager, outTargetID);
}

glm::vec3 GetFeet(EntityManager& entityManager, unsigned int entityID)
{
	const TransformComponent* transform = entityManager.GetComponent<TransformComponent>(entityID);
	return transform ? transform->position : glm::vec3(0.0f);
}

glm::vec3 GetAimPoint(EntityManager& entityManager, unsigned int entityID)
{
	glm::vec3 aimPoint;
	if (LockOnSystem::TryGetAimPointOf(entityManager, entityID, aimPoint))
		return aimPoint;

	return GetFeet(entityManager, entityID) + WorldUp * DefaultAimHeight;
}

// ========== Spells of a character ==========

// The closest entity with Character Actions, going up from the spell through its parents
bool TryFindCaster(EntityManager& entityManager, unsigned int spellEntityID, unsigned int& outCasterID)
{
	unsigned int entityID = spellEntityID;

	while (!entityManager.GetComponent<CharacterActionsComponent>(entityID))
	{
		if (!EntityHierarchy::TryGetParent(entityManager, entityID, entityID))
			return false;
	}

	outCasterID = entityID;
	return true;
}

// The spells among the character's children (and their children: a spell on the tip of a staff in the hand)
void CollectSpells(EntityManager& entityManager, unsigned int entityID, std::vector<unsigned int>& outSpellIDs)
{
	for (unsigned int childID : EntityHierarchy::GetChildren(entityManager, entityID))
	{
		if (entityManager.GetComponent<SpellCasterComponent>(childID))
			outSpellIDs.push_back(childID);

		CollectSpells(entityManager, childID, outSpellIDs);
	}
}

bool FitsDistance(const SpellCasterComponent& spell, float distance)
{
	return spell.IsReady() && !spell.isCastOnRequestOnly && distance >= spell.minDistance && distance <= spell.maxDistance;
}

glm::vec3 GetWorldPosition(EntityManager& entityManager, unsigned int entityID)
{
	if (const WorldMatrixComponent* worldMatrix = entityManager.GetComponent<WorldMatrixComponent>(entityID))
		return glm::vec3(worldMatrix->matrix[3]);

	return GetFeet(entityManager, entityID);
}

// ========== Throwing ==========

void LaunchProjectile(EntityManager& entityManager, unsigned int templateID, unsigned int casterID, unsigned int targetID, bool hasTarget,
	const glm::vec3& from, const glm::vec3& direction)
{
	static unsigned int lastNumber = 0;

	LiveProjectile projectile;
	projectile.number       = ++lastNumber;
	projectile.templateID   = templateID;
	projectile.casterID     = casterID;
	projectile.targetID     = targetID;
	projectile.hasTarget    = hasTarget;
	projectile.isPlayerSide = IsOnPlayersSide(entityManager, casterID);
	projectile.position     = from;
	projectile.direction    = glm::length(direction) > SmallestUsableLength ? glm::normalize(direction) : DefaultForward;

	if (hasTarget)
		projectile.aimOffset = GetAimPoint(entityManager, targetID) - GetFeet(entityManager, targetID);

	projectile.hasBody = TryBorrowBody(entityManager, templateID, projectile.bodyID);
	if (projectile.hasBody)
	{
		if (const TransformComponent* body = entityManager.GetComponent<TransformComponent>(projectile.bodyID))
		{
			projectile.bodyScale    = body->scale;
			projectile.bodyRotation = body->rotation;
		}

		if (const ProjectileComponent* settings = entityManager.GetComponent<ProjectileComponent>(templateID))
			CarryBody(entityManager, *settings, projectile);
		ShowBody(entityManager, projectile.bodyID, true);
	}

	GetLiveProjectiles().push_back(projectile);
}

// The projectiles leave: flying at the target fanned out, or appearing under it
void Release(EntityManager& entityManager, unsigned int spellID, const SpellCasterComponent& spell, unsigned int casterID)
{
	unsigned int templateID = 0;
	if (!TryFindEntityByName(entityManager, spell.projectile, templateID) || !entityManager.GetComponent<ProjectileComponent>(templateID))
		return;

	const ProjectileComponent& projectile = *entityManager.GetComponent<ProjectileComponent>(templateID);
	const glm::vec3 from = GetWorldPosition(entityManager, spellID);

	unsigned int targetID = 0;
	const bool hasTarget  = TryFindTarget(entityManager, casterID, targetID);

	const CharacterActionsComponent* actions = entityManager.GetComponent<CharacterActionsComponent>(casterID);
	const glm::vec3 facing = actions ? actions->actionDirection : DefaultForward;

	const int count = std::max(spell.projectileCount, 1);

	if (spell.dropHeight > 0.0f)
	{
		// From the sky, straight down on where the target stands now
		const glm::vec3 below = hasTarget ? GetFeet(entityManager, targetID) : GetFeet(entityManager, casterID) + facing * spell.minDistance;
		for (int i = 0; i < count; ++i)
			LaunchProjectile(entityManager, templateID, casterID, targetID, hasTarget, below + WorldUp * spell.dropHeight, -WorldUp);
	}
	else if (spell.atTargetsFeet)
	{
		const glm::vec3 center = hasTarget ? GetFeet(entityManager, targetID) : GetFeet(entityManager, casterID) + facing * spell.minDistance;
		const float     ring   = projectile.radius * FeetRingSpacing;

		LaunchProjectile(entityManager, templateID, casterID, targetID, hasTarget, center, facing);
		for (int i = 1; i < count; ++i)
		{
			const float angle = glm::two_pi<float>() * (i - 1) / std::max(count - 1, 1);
			LaunchProjectile(entityManager, templateID, casterID, targetID, hasTarget, center + glm::vec3(std::cos(angle), 0.0f, std::sin(angle)) * ring, facing);
		}
	}
	else
	{
		const glm::vec3 aim = hasTarget ? GetAimPoint(entityManager, targetID) - from : facing;

		for (int i = 0; i < count; ++i)
		{
			const float degrees = spell.spreadDegrees * (i - (count - 1) * 0.5f);
			LaunchProjectile(entityManager, templateID, casterID, targetID, hasTarget, from, glm::angleAxis(glm::radians(degrees), WorldUp) * aim);
		}
	}

	if (!spell.castSound.empty())
		AudioSystem::PlayAtEntity(entityManager, casterID, spell.castSound);
}

// The wind-up glows where the spell leaves from, then the projectiles leave at their moment.
// A hit that staggered the caster (or anything else that ended the cast) loses the spell
void UpdateSpell(EntityManager& entityManager, unsigned int spellID, SpellCasterComponent& spell, float deltaTime)
{
	spell.cooldownLeft = std::max(spell.cooldownLeft - deltaTime, 0.0f);
	if (!spell.isCasting)
		return;

	unsigned int casterID = 0;
	const CharacterActionsComponent* actions = TryFindCaster(entityManager, spellID, casterID)
		? entityManager.GetComponent<CharacterActionsComponent>(casterID) : nullptr;

	if (!actions || actions->currentAction != CharacterAction::Cast || actions->castAnimation != spell.castAnimation)
	{
		spell.isCasting = false;
		return;
	}

	if (spell.hasReleased)
		return;

	if (actions->actionProgress >= spell.releaseAt)
	{
		spell.hasReleased = true;
		++spell.timesReleased;
		Release(entityManager, spellID, spell, casterID);
		return;
	}

	if (spell.chargeEffect.empty())
		return;

	spell.unbornChargeParticles += spell.chargeParticlesPerSecond * deltaTime;
	const int count = static_cast<int>(spell.unbornChargeParticles);
	spell.unbornChargeParticles -= count;
	if (count > 0)
		ParticleSystem::BurstAt(entityManager, spell.chargeEffect, GetWorldPosition(entityManager, spellID), count);
}

// The cast starts as an action of the caster; the spell rests for its cooldown whether it leaves or is lost to a hit
bool StartCast(EntityManager& entityManager, unsigned int casterID, SpellCasterComponent& spell, bool isForced = false)
{
	if (!CharacterActionsSystem::Cast(entityManager, casterID, spell.castAnimation, spell.releaseAt, isForced))
		return false;

	if (CharacterActionsComponent* actions = entityManager.GetComponent<CharacterActionsComponent>(casterID))
		actions->isCastUninterruptible = spell.isUninterruptible;

	spell.isCasting             = true;
	spell.hasReleased           = false;
	spell.unbornChargeParticles = 0.0f;
	spell.cooldownLeft          = spell.cooldownSeconds;
	return true;
}

// ========== The player's keys ==========

// Letters and digits have the code of their (upper case) character
int GetKeyCode(const std::string& castKey)
{
	return std::toupper(static_cast<unsigned char>(castKey.front()));
}

void CastSpellsOfPressedKeys(EntityManager& entityManager)
{
	if (CutsceneSystem::IsPlaying(entityManager))
		return;

	for (SpellCasterComponent& spell : entityManager.GetAllComponents<SpellCasterComponent>())
	{
		if (spell.castKey.empty() || !InputManager::IsKeyDown(GetKeyCode(spell.castKey)))
			continue;

		const unsigned int spellID = entityManager.GetEntityIDForComponent(spell);
		unsigned int casterID = 0;

		if (TryFindCaster(entityManager, spellID, casterID) && IsOnPlayersSide(entityManager, casterID))
			SpellSystem::TryCast(entityManager, spellID);
	}
}

// ========== Flight ==========

// The point of the box closest to 'point'
glm::vec3 GetClosestPoint(const OBB& box, const glm::vec3& point)
{
	const glm::vec3 local = glm::clamp(glm::transpose(box.orientation) * (point - box.center), -box.halfExtents, box.halfExtents);
	return box.center + box.orientation * local;
}

void Burst(EntityManager& entityManager, const ProjectileComponent& settings, const glm::vec3& point, unsigned int casterID)
{
	settings.timesBurst++;
	settings.lastBurstPosition = point;

	if (!settings.hitEffect.empty())
		ParticleSystem::BurstAt(entityManager, settings.hitEffect, point, settings.hitBurst);

	if (!settings.hitSound.empty())
		AudioSystem::PlayAtEntity(entityManager, casterID, settings.hitSound);
}

// A projectile with a fuse: everyone of the other side close enough is hurt at once
void Explode(EntityManager& entityManager, const ProjectileComponent& settings, const LiveProjectile& projectile)
{
	Burst(entityManager, settings, projectile.position, projectile.casterID);
	FollowCameraSystem::ShakeFrom(entityManager, settings.cameraShake, projectile.position, settings.radius * BurstShakeReach);

	for (HealthComponent& health : entityManager.GetAllComponents<HealthComponent>())
	{
		const unsigned int targetID = entityManager.GetEntityIDForComponent(health);
		if (health.isDead || targetID == projectile.casterID || IsOnPlayersSide(entityManager, targetID) == projectile.isPlayerSide)
			continue;

		const glm::vec3 offset = GetFeet(entityManager, targetID) - projectile.position;
		if (glm::length(glm::vec2(offset.x, offset.z)) <= settings.radius && std::abs(offset.y) <= BurstHeight)
			CombatSystem::HitWithSpell(entityManager, targetID, settings.damage, settings.poiseDamage, projectile.casterID,
				GetAimPoint(entityManager, targetID), settings.hitStopSeconds, 0.0f);
	}
}

// Turns after the target, no faster than its homing allows
void Steer(EntityManager& entityManager, const ProjectileComponent& settings, LiveProjectile& projectile, float deltaTime)
{
	if (settings.homingDegreesPerSecond <= 0.0f || !projectile.hasTarget)
		return;

	const HealthComponent* health = entityManager.GetComponent<HealthComponent>(projectile.targetID);
	if (!health || health->isDead)
		return;

	const glm::vec3 toTarget = GetFeet(entityManager, projectile.targetID) + projectile.aimOffset - projectile.position;
	if (glm::length(toTarget) < SmallestUsableLength)
		return;

	const glm::vec3 wanted = glm::normalize(toTarget);
	const float     angle  = std::acos(glm::clamp(glm::dot(projectile.direction, wanted), -1.0f, 1.0f));
	const float     most   = glm::radians(settings.homingDegreesPerSecond) * deltaTime;
	if (angle <= most)
	{
		projectile.direction = wanted;
		return;
	}

	const glm::vec3 axis = glm::cross(projectile.direction, wanted);
	if (glm::length(axis) < SmallestUsableLength)
		return;

	projectile.direction = glm::normalize(glm::angleAxis(most, glm::normalize(axis)) * projectile.direction);
}

// The first character of the other side the projectile touches. False when there is none (or it rolls through)
bool TryHitCharacter(EntityManager& entityManager, const ProjectileComponent& settings, LiveProjectile& projectile)
{
	for (HealthComponent& health : entityManager.GetAllComponents<HealthComponent>())
	{
		const unsigned int targetID = entityManager.GetEntityIDForComponent(health);
		const BoxColliderComponent* body = entityManager.GetComponent<BoxColliderComponent>(targetID);

		if (!body || health.isDead || targetID == projectile.casterID || IsOnPlayersSide(entityManager, targetID) == projectile.isPlayerSide)
			continue;

		const std::vector<unsigned int>& passed = projectile.passedThrough;
		if (std::find(passed.begin(), passed.end(), targetID) != passed.end())
			continue;

		const glm::vec3 closest = GetClosestPoint(body->worldOBB, projectile.position);
		if (glm::length(closest - projectile.position) > settings.radius)
			continue;

		if (CombatSystem::HitWithSpell(entityManager, targetID, settings.damage, settings.poiseDamage, projectile.casterID,
			closest, settings.hitStopSeconds, settings.cameraShake))
		{
			Burst(entityManager, settings, closest, projectile.casterID);
			return true;
		}

		projectile.passedThrough.push_back(targetID);
	}

	return false;
}

void LeaveTrail(EntityManager& entityManager, const ProjectileComponent& settings, LiveProjectile& projectile, float deltaTime)
{
	if (settings.trailEffect.empty())
		return;

	projectile.unbornTrailParticles += settings.trailParticlesPerSecond * deltaTime;
	const int count = static_cast<int>(projectile.unbornTrailParticles);
	projectile.unbornTrailParticles -= count;
	if (count > 0)
		ParticleSystem::BurstAt(entityManager, settings.trailEffect, projectile.position, count);
}

// True while the projectile hangs where it appeared, growing (see ProjectileComponent::launchDelaySeconds).
// The moment that ends it turns to where its target is now
bool IsStillForming(EntityManager& entityManager, const ProjectileComponent& settings, LiveProjectile& projectile, float deltaTime)
{
	if (settings.launchDelaySeconds <= 0.0f || projectile.hasLaunched)
		return false;

	if (projectile.age < settings.launchDelaySeconds)
	{
		CarryBody(entityManager, settings, projectile);
		LeaveTrail(entityManager, settings, projectile, deltaTime);
		return true;
	}

	projectile.hasLaunched = true;

	if (projectile.hasTarget && settings.aimsAfterForming)
	{
		const glm::vec3 toTarget = GetFeet(entityManager, projectile.targetID) + projectile.aimOffset - projectile.position;
		if (glm::length(toTarget) > SmallestUsableLength)
			projectile.direction = glm::normalize(toTarget);
	}

	return false;
}

// True while the projectile goes on
bool UpdateProjectile(EntityManager& entityManager, LiveProjectile& projectile, float deltaTime)
{
	const ProjectileComponent* settings = entityManager.GetComponent<ProjectileComponent>(projectile.templateID);
	if (!settings)
		return false;

	projectile.age += deltaTime;

	if (IsStillForming(entityManager, *settings, projectile, deltaTime))
		return true;

	if (settings->fuseSeconds > 0.0f && projectile.age >= settings->fuseSeconds + settings->launchDelaySeconds)
	{
		Explode(entityManager, *settings, projectile);
		return false;
	}

	if (projectile.age >= settings->lifetimeSeconds + settings->launchDelaySeconds && settings->fuseSeconds <= 0.0f)
		return false;

	Steer(entityManager, *settings, projectile, deltaTime);

	// A wall on the way stops it there
	const float step = settings->speed * deltaTime;
	if (step > 0.0f)
	{
		Ray ray;
		ray.origin    = projectile.position;
		ray.direction = projectile.direction;

		RaycastHit hit;
		if (Physics::Raycast(entityManager, ray, step, SightBlockingLayers, projectile.casterID, hit))
		{
			projectile.position = hit.point;
			if (settings->fuseSeconds > 0.0f)
				Explode(entityManager, *settings, projectile);
			else
				Burst(entityManager, *settings, hit.point, projectile.casterID);
			return false;
		}

		projectile.position += projectile.direction * step;
	}

	CarryBody(entityManager, *settings, projectile);
	LeaveTrail(entityManager, *settings, projectile, deltaTime);

	return settings->fuseSeconds > 0.0f || !TryHitCharacter(entityManager, *settings, projectile);
}

}

void SpellSystem::Clear(EntityManager& entityManager)
{
	GetLiveProjectiles().clear();
	HideAllBodies(entityManager);

	for (SpellCasterComponent& spell : entityManager.GetAllComponents<SpellCasterComponent>())
	{
		spell.cooldownLeft          = 0.0f;
		spell.isCasting             = false;
		spell.hasReleased           = false;
		spell.unbornChargeParticles = 0.0f;
		spell.timesReleased         = 0;
	}
}

void SpellSystem::Update(EntityManager& entityManager, float deltaTime)
{
	CastSpellsOfPressedKeys(entityManager);

	for (SpellCasterComponent& spell : entityManager.GetAllComponents<SpellCasterComponent>())
		UpdateSpell(entityManager, entityManager.GetEntityIDForComponent(spell), spell, deltaTime);

	std::vector<LiveProjectile>& projectiles = GetLiveProjectiles();
	// One by one, not with remove_if: a projectile that is gone gives its body back before the next one is looked at
	for (size_t i = 0; i < projectiles.size();)
	{
		if (UpdateProjectile(entityManager, projectiles[i], deltaTime))
		{
			++i;
			continue;
		}

		ReturnBody(entityManager, projectiles[i]);
		projectiles.erase(projectiles.begin() + i);
	}
}

bool SpellSystem::TryCastSpell(EntityManager& entityManager, unsigned int casterEntityID, float distanceToTarget)
{
	std::vector<unsigned int> spellIDs;
	CollectSpells(entityManager, casterEntityID, spellIDs);

	// A different spell each time when several fit (the first ready one in a random order)
	std::vector<unsigned int> fitting;
	for (unsigned int spellID : spellIDs)
	{
		if (FitsDistance(*entityManager.GetComponent<SpellCasterComponent>(spellID), distanceToTarget))
			fitting.push_back(spellID);
	}
	if (fitting.empty())
		return false;

	const unsigned int spellID = fitting[static_cast<size_t>(std::rand()) % fitting.size()];
	return StartCast(entityManager, casterEntityID, *entityManager.GetComponent<SpellCasterComponent>(spellID));
}

bool SpellSystem::TryCast(EntityManager& entityManager, unsigned int spellEntityID, bool isForced)
{
	SpellCasterComponent* spell = entityManager.GetComponent<SpellCasterComponent>(spellEntityID);
	unsigned int casterID = 0;

	if (!spell || (!isForced && !spell->IsReady()) || !TryFindCaster(entityManager, spellEntityID, casterID))
		return false;

	return StartCast(entityManager, casterID, *spell, isForced);
}

bool SpellSystem::Launch(EntityManager& entityManager, const std::string& projectileName, unsigned int casterEntityID,
	const glm::vec3& from, const glm::vec3& direction)
{
	unsigned int templateID = 0;
	if (!TryFindEntityByName(entityManager, projectileName, templateID) || !entityManager.GetComponent<ProjectileComponent>(templateID))
		return false;

	unsigned int targetID = 0;
	const bool hasTarget  = TryFindTarget(entityManager, casterEntityID, targetID);

	LaunchProjectile(entityManager, templateID, casterEntityID, targetID, hasTarget, from, direction);
	return true;
}

bool SpellSystem::TryFindIncomingProjectile(EntityManager& entityManager, unsigned int entityID, float withinSeconds,
	unsigned int& outProjectileNumber)
{
	const bool      isPlayerSide = IsOnPlayersSide(entityManager, entityID);
	const glm::vec3 aimPoint     = GetAimPoint(entityManager, entityID);

	for (const LiveProjectile& projectile : GetLiveProjectiles())
	{
		const ProjectileComponent* settings = entityManager.GetComponent<ProjectileComponent>(projectile.templateID);
		if (!settings || projectile.isPlayerSide == isPlayerSide || settings->fuseSeconds > 0.0f || settings->speed <= 0.0f)
			continue;

		// Coming closer, and close enough to arrive in time
		const glm::vec3 toCharacter = aimPoint - projectile.position;
		const float     distance    = glm::length(toCharacter);
		if (settings->launchDelaySeconds > 0.0f && !projectile.hasLaunched)
			continue;
		if (distance < SmallestUsableLength || glm::dot(toCharacter / distance, projectile.direction) < IncomingMinCosine)
			continue;

		if (distance / settings->speed <= withinSeconds)
		{
			outProjectileNumber = projectile.number;
			return true;
		}
	}

	return false;
}

bool SpellSystem::HasSpellReady(EntityManager& entityManager, unsigned int casterEntityID, float distanceToTarget)
{
	std::vector<unsigned int> spellIDs;
	CollectSpells(entityManager, casterEntityID, spellIDs);

	return std::any_of(spellIDs.begin(), spellIDs.end(), [&](unsigned int spellID)
		{ return FitsDistance(*entityManager.GetComponent<SpellCasterComponent>(spellID), distanceToTarget); });
}

void SpellSystem::ReportProblemsBeforePlay(EntityManager& entityManager)
{
	for (SpellCasterComponent& spell : entityManager.GetAllComponents<SpellCasterComponent>())
	{
		const unsigned int spellID = entityManager.GetEntityIDForComponent(spell);
		const std::string  name    = GetEntityName(entityManager, spellID);

		unsigned int casterID = 0;
		if (!TryFindCaster(entityManager, spellID, casterID))
			SUNTA_ENGINE_LOG_WARNING("Spell Caster on '{0}' has no caster with Character Actions (make it a child of the character), it will never be cast", name);

		unsigned int templateID = 0;
		if (!TryFindEntityByName(entityManager, spell.projectile, templateID) || !entityManager.GetComponent<ProjectileComponent>(templateID))
			SUNTA_ENGINE_LOG_WARNING("Spell Caster on '{0}': there is no entity named '{1}' with a Projectile component, the spell throws nothing", name, spell.projectile);
	}
}

}
