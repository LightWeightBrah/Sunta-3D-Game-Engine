#include "Core/SuntaPreCompiled.h"
#include "BonfireSetup.h"

#include "CharacterSetup.h"
#include "Core/EngineAssets.h"
#include "ECS/Component.h"
#include "ECS/EntityManager.h"

namespace Sunta
{

namespace
{

constexpr const char* LightName = "Bonfire Light";

// Just above the flames
const glm::vec3 LightOffset = glm::vec3(0.0f, 0.7f, 0.0f);

// A warm orange light that reaches about ten meters
const glm::vec3 LightAmbient  = glm::vec3(0.05f, 0.025f, 0.01f);
const glm::vec3 LightDiffuse  = glm::vec3(1.0f, 0.55f, 0.22f);
const glm::vec3 LightSpecular = glm::vec3(0.8f, 0.5f, 0.25f);
constexpr float LightLinear    = 0.14f;
constexpr float LightQuadratic = 0.07f;

// The stones and the fire, so the player stops at the edge of the ring
const glm::vec3 BodyHalfExtents = glm::vec3(0.6f, 0.35f, 0.6f);

template<typename T>
bool Has(EntityManager& entityManager, unsigned int entityID)
{
	return entityManager.GetComponent<T>(entityID) != nullptr;
}

void AddBody(EntityManager& entityManager, unsigned int entityID)
{
	if (Has<BoxColliderComponent>(entityManager, entityID))
		return;

	BoxColliderComponent& body = entityManager.AddComponent<BoxColliderComponent>(entityID);
	body.halfExtents = BodyHalfExtents;
	body.localOffset = glm::vec3(0.0f, BodyHalfExtents.y, 0.0f);
}

void AddLight(EntityManager& entityManager, unsigned int bonfireID)
{
	const unsigned int lightID = CharacterSetup::CreateNamedEntity(entityManager, LightName, LightOffset, glm::vec3(0.0f));

	PointLightComponent& light = entityManager.AddComponent<PointLightComponent>(lightID);
	light.color.ambientIntensity  = LightAmbient;
	light.color.diffuseIntensity  = LightDiffuse;
	light.color.specularIntensity = LightSpecular;
	light.attenuation.linear      = LightLinear;
	light.attenuation.quadratic   = LightQuadratic;

	entityManager.AddComponent<ParentComponent>(lightID).parentID = bonfireID;
}

}

void BonfireSetup::SetUp(EntityManager& entityManager, unsigned int entityID)
{
	if (!Has<TransformComponent>(entityManager, entityID) || IsSetUp(entityManager, entityID))
		return;

	CharacterSetup::AddModelIfMissing(entityManager, entityID, EngineAssets::Models::Bonfire);
	entityManager.AddComponent<BonfireComponent>(entityID);

	AddBody(entityManager, entityID);
	AddLight(entityManager, entityID);
}

bool BonfireSetup::IsSetUp(EntityManager& entityManager, unsigned int entityID)
{
	return Has<BonfireComponent>(entityManager, entityID);
}

}
