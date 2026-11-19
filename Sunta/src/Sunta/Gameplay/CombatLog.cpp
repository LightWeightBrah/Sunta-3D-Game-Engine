#include "Core/SuntaPreCompiled.h"
#include "CombatLog.h"

#include "CharacterActionsComponent.h"
#include "EntityName.h"
#include "Core/InputPlayback.h"
#include "ECS/Component.h"
#include "ECS/EntityManager.h"

namespace Sunta
{

namespace
{

constexpr float NearbyDistance	= 3.0f; // swings farther than this from a target aren't about that target
constexpr float SecondsToFinish = 2.5f; // a swing's line is written this long after it began at the latest

struct SwingRecord
{
	std::string	 ownerName;
	std::string	 targetName;
	std::string	 animation;
	unsigned int ownerID	  = 0;
	unsigned int attackNumber = 0;
	unsigned int targetID	  = 0;
	float		 startTime	  = 0.0f;
	float		 closestGap	  = 1e9f;
	float		 heightAtClosest = 0.0f; // 0 = target's feet, 1 = top of its body box
	float		 sideAtClosest	 = 0.0f; // weapon's sideways offset from the line owner -> target (m, + = right)
	float		 distanceAtClosest = 0.0f; // between the two characters (m, on the ground)
	std::string	 weapon; // the hitbox that came closest
	std::string	 result;
	float		 damage		  = 0.0f;
	float		 contactHeight = -1.0f;
};

bool					 isOn	   = false;
bool					 isChecked = false;
float					 now	   = 0.0f;
std::ofstream			 file;
std::vector<SwingRecord> open;

float HalfHeight(const OBB& box)
{
	return std::abs(box.orientation[0].y) * box.halfExtents.x
		 + std::abs(box.orientation[1].y) * box.halfExtents.y
		 + std::abs(box.orientation[2].y) * box.halfExtents.z;
}

float HeightIn(const OBB& body, float y)
{
	const float half = HalfHeight(body);
	return half <= 0.0f ? 0.0f : (y - (body.center.y - half)) / (2.0f * half);
}

glm::vec3 ClosestPointIn(const OBB& box, const glm::vec3& point)
{
	const glm::vec3 local = glm::clamp(glm::transpose(box.orientation) * (point - box.center), -box.halfExtents, box.halfExtents);
	return box.center + box.orientation * local;
}

glm::vec3 PositionOf(EntityManager& entityManager, unsigned int entityID)
{
	const WorldMatrixComponent* world = entityManager.GetComponent<WorldMatrixComponent>(entityID);
	return world ? glm::vec3(world->matrix[3]) : glm::vec3(0.0f);
}

void Write(const SwingRecord& r)
{
	if (!file)
		return;

	file << std::fixed << std::setprecision(2)
		 << r.startTime << ",swing," << r.ownerName << "," << r.targetName << "," << r.animation << "," << r.weapon << ","
		 << (r.result.empty() ? "miss" : r.result) << "," << r.damage << ","
		 << r.closestGap << "," << r.heightAtClosest << "," << r.sideAtClosest << "," << r.distanceAtClosest << ","
		 << r.contactHeight << "\n";
	file.flush();
}

SwingRecord& Find(EntityManager& entityManager, unsigned int ownerID, unsigned int attackNumber, unsigned int targetID)
{
	for (SwingRecord& r : open)
		if (r.ownerID == ownerID && r.attackNumber == attackNumber && r.targetID == targetID)
			return r;

	SwingRecord r;
	r.ownerID	   = ownerID;
	r.attackNumber = attackNumber;
	r.targetID	   = targetID;
	r.startTime	   = now;
	r.ownerName	   = GetEntityName(entityManager, ownerID);
	r.targetName   = GetEntityName(entityManager, targetID);
	if (const CharacterActionsComponent* actions = entityManager.GetComponent<CharacterActionsComponent>(ownerID))
		r.animation = actions->currentAnimation;

	open.push_back(r);
	return open.back();
}

}

bool CombatLog::IsOn()
{
	if (!isChecked)
	{
		isChecked = true;
		const char* value = std::getenv("SUNTA_COMBAT_LOG");
		isOn = InputPlayback::IsActive() || (value && *value && *value != '0');
		if (isOn)
		{
			const std::filesystem::path path = std::filesystem::path(InputPlayback::GetOutputFolder()) / "combat_log.csv";
			file.open(path);
			file << "time,kind,attacker,target,animation,weapon,result,damage,closest_gap_m,height_01,side_m,distance_m,contact_height_01\n";
		}
	}
	return isOn;
}

void CombatLog::Advance(float deltaTime)
{
	if (!IsOn())
		return;

	now += deltaTime;

	// A swing is finished when its owner started another one, or after a while
	for (auto it = open.begin(); it != open.end();)
	{
		bool finished = now - it->startTime > SecondsToFinish;
		for (const SwingRecord& other : open)
			if (other.ownerID == it->ownerID && other.attackNumber != it->attackNumber && other.startTime > it->startTime)
				finished = true;

		if (finished)
		{
			Write(*it);
			it = open.erase(it);
		}
		else
			++it;
	}
}

void CombatLog::TrackSwing(EntityManager& entityManager, unsigned int ownerID, unsigned int attackNumber,
	unsigned int targetID, unsigned int weaponID, const OBB& weapon, const OBB& targetBody)
{
	if (!IsOn())
		return;

	const glm::vec3 closest = ClosestPointIn(targetBody, weapon.center);
	const float		gap		= glm::length(closest - weapon.center);
	if (gap > NearbyDistance)
		return;

	SwingRecord& r = Find(entityManager, ownerID, attackNumber, targetID);
	if (gap >= r.closestGap)
		return;

	const glm::vec3 owner  = PositionOf(entityManager, ownerID);
	const glm::vec3 target = PositionOf(entityManager, targetID);
	glm::vec3 forward = target - owner;
	forward.y = 0.0f;
	const float distance = glm::length(forward);
	forward = distance > 0.001f ? forward / distance : glm::vec3(0.0f, 0.0f, 1.0f);
	const glm::vec3 right = glm::normalize(glm::cross(forward, glm::vec3(0.0f, 1.0f, 0.0f)));

	r.weapon			= GetEntityName(entityManager, weaponID);
	r.closestGap		= gap;
	r.heightAtClosest	= HeightIn(targetBody, weapon.center.y);
	r.sideAtClosest		= glm::dot(weapon.center - targetBody.center, right);
	r.distanceAtClosest = distance;
}

void CombatLog::RecordHit(EntityManager& entityManager, unsigned int ownerID, unsigned int attackNumber,
	unsigned int targetID, const char* result, float damage, const glm::vec3& contactPoint, const OBB& targetBody)
{
	if (!IsOn())
		return;

	SwingRecord& r = Find(entityManager, ownerID, attackNumber, targetID);
	if (!r.result.empty())
		return;

	r.result		= result;
	r.damage		= damage;
	r.contactHeight = HeightIn(targetBody, contactPoint.y);
}

bool CombatLog::IsTracing()
{
	static const bool isTracing = []
	{
		const char* value = std::getenv("SUNTA_COMBAT_TRACE");
		return value && *value && *value != '0';
	}();
	return isTracing && IsOn();
}

void CombatLog::Trace(EntityManager& entityManager, unsigned int ownerID, unsigned int weaponID, const OBB& weapon,
	float progress, bool isInHitWindow)
{
	if (!IsTracing() || !file)
		return;

	const WorldMatrixComponent* world = entityManager.GetComponent<WorldMatrixComponent>(ownerID);
	const CharacterActionsComponent* actions = entityManager.GetComponent<CharacterActionsComponent>(ownerID);
	if (!world || !actions)
		return;

	const glm::vec3 owner	= glm::vec3(world->matrix[3]);
	glm::vec3		forward = -glm::vec3(world->matrix[2]); // an entity looks along -Z
	forward.y = 0.0f;
	forward = glm::length(forward) > 0.0001f ? glm::normalize(forward) : glm::vec3(0.0f, 0.0f, 1.0f);

	file << std::fixed << std::setprecision(3) << now << ",trace," << GetEntityName(entityManager, ownerID) << ","
		 << actions->currentAnimation << "," << GetEntityName(entityManager, weaponID) << "," << progress << ","
		 << (weapon.center.y - owner.y) << "," << glm::dot(weapon.center - owner, forward) << "," << (isInHitWindow ? 1 : 0) << '\n';
}

void CombatLog::Event(const std::string& text)
{
	if (!IsOn() || !file)
		return;

	file << std::fixed << std::setprecision(2) << now << ",event," << text << "\n";
	file.flush();
}

void CombatLog::Flush()
{
	for (const SwingRecord& r : open)
		Write(r);
	open.clear();
}

}
