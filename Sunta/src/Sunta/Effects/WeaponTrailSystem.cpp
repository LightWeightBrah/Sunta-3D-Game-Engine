#include "Core/SuntaPreCompiled.h"
#include "WeaponTrailSystem.h"

#include <algorithm>

#include "ECS/Component.h"
#include "ECS/EntityManager.h"
#include "Core/VirtualFileSystem.h"
#include "Renderer/ElementBuffer.h"
#include "Renderer/Renderer.h"
#include "Renderer/RendererDevice.h"
#include "Renderer/ScopedVertexArrayBind.h"
#include "Renderer/Shader.h"
#include "Renderer/VertexArray.h"
#include "Renderer/VertexBuffer.h"
#include "Renderer/VertexLayouts.h"
#include "Scene/EntityHierarchy.h"

namespace Sunta
{

namespace
{

constexpr const char* ShaderPath = "@engine/Shaders/Trail.shader";

// The trail starts a little before the striking part of an attack (the blade is already swinging) and ends a little after
constexpr float StartBeforeHit = 0.08f;
constexpr float EndAfterHit    = 0.06f;

// One piece of ribbon: its corners are picked in the vertex shader from these corners of the square
// (x: base or tip of the blade, y: the older or the newer record)
const glm::vec3 QuadCorners[] =
{
	{ -0.5f, -0.5f, 0.0f },
	{  0.5f, -0.5f, 0.0f },
	{  0.5f,  0.5f, 0.0f },
	{ -0.5f,  0.5f, 0.0f }
};

const unsigned int QuadIndices[] = { 0, 1, 2, 2, 3, 0 };

// The closest entity with Character Actions, going up from the weapon through its parents (the hand holding it)
const CharacterActionsComponent* FindOwner(EntityManager& entityManager, unsigned int weaponEntityID)
{
	unsigned int entityID = weaponEntityID;

	while (true)
	{
		if (const CharacterActionsComponent* actions = entityManager.GetComponent<CharacterActionsComponent>(entityID))
			return actions;

		if (!EntityHierarchy::TryGetParent(entityManager, entityID, entityID))
			return nullptr;
	}
}

bool IsSwinging(const CharacterActionsComponent* owner)
{
	if (!owner || !owner->IsAttacking())
		return false;

	const float from = owner->GetHitFrom() - StartBeforeHit;
	float to = owner->lightHitTo;
	if (owner->currentAction == CharacterAction::HeavyAttack) to = owner->heavyHitTo;
	if (owner->currentAction == CharacterAction::LeapAttack)  to = owner->leapHitTo;

	return owner->actionProgress >= from && owner->actionProgress <= to + EndAfterHit;
}

void AgeSamples(WeaponTrailComponent& trail, float deltaTime)
{
	for (WeaponTrailComponent::Sample& sample : trail.samples)
		sample.age += deltaTime;

	auto isOld = [&trail](const WeaponTrailComponent::Sample& sample) { return sample.age >= trail.lifetimeSeconds; };
	trail.samples.erase(std::remove_if(trail.samples.begin(), trail.samples.end(), isOld), trail.samples.end());
}

}

void WeaponTrailSystem::Update(EntityManager& entityManager, float deltaTime)
{
	for (WeaponTrailComponent& trail : entityManager.GetAllComponents<WeaponTrailComponent>())
	{
		AgeSamples(trail, deltaTime);

		const unsigned int entityID = entityManager.GetEntityIDForComponent(trail);
		if (trail.onlyWhileStriking && !IsSwinging(FindOwner(entityManager, entityID)))
			continue;

		const WorldMatrixComponent* world = entityManager.GetComponent<WorldMatrixComponent>(entityID);
		if (!world)
			continue;

		const glm::vec3 base = glm::vec3(world->matrix * glm::vec4(trail.baseOffset, 1.0f));
		const glm::vec3 tip  = glm::vec3(world->matrix * glm::vec4(trail.tipOffset, 1.0f));
		trail.samples.push_back({ base, tip, 0.0f });
	}
}

void WeaponTrailSystem::Clear(EntityManager& entityManager)
{
	for (WeaponTrailComponent& trail : entityManager.GetAllComponents<WeaponTrailComponent>())
		trail.samples.clear();
}

void WeaponTrailSystem::EnsureInitialized()
{
	if (shader)
		return;

	RendererDevice& rendererDevice = Renderer::GetDevice();

	quad = rendererDevice.CreateVertexArray();
	ScopedVertexArrayBind vertexArrayGuard(*quad);

	BufferDescriptor vertexBufferDescriptor;
	vertexBufferDescriptor.data = QuadCorners;
	vertexBufferDescriptor.size = sizeof(QuadCorners);

	auto vertexBuffer = rendererDevice.CreateVertexBuffer(vertexBufferDescriptor);
	vertexBuffer->SetLayout(VertexLayouts::GetPositionOnlyLayout());

	BufferDescriptor elementBufferDescriptor;
	elementBufferDescriptor.data = QuadIndices;
	elementBufferDescriptor.size = sizeof(QuadIndices);

	quad->AddVertexBuffer(vertexBuffer);
	quad->SetElementBuffer(rendererDevice.CreateElementBuffer(elementBufferDescriptor));

	shader = rendererDevice.CreateShader(VirtualFileSystem::Resolve(ShaderPath));
}

void WeaponTrailSystem::Draw(EntityManager& entityManager, const glm::mat4& view, const glm::mat4& projection)
{
	auto& trails = entityManager.GetAllComponents<WeaponTrailComponent>();

	bool hasStreaks = false;
	for (const WeaponTrailComponent& trail : trails)
		hasStreaks = hasStreaks || trail.samples.size() >= 2;

	if (!hasStreaks)
		return;

	EnsureInitialized();

	RendererDevice& rendererDevice = Renderer::GetDevice();
	rendererDevice.SetAdditiveBlending(true);
	rendererDevice.SetDepthWrite(false);

	shader->Bind();
	shader->SetUniformMatrix4fv("viewProjection", projection * view);

	for (const WeaponTrailComponent& trail : trails)
	{
		const glm::vec3 color = trail.color * trail.intensity;
		shader->SetUniform3f("color", color.r, color.g, color.b);

		for (size_t i = 1; i < trail.samples.size(); i++)
		{
			const WeaponTrailComponent::Sample& older = trail.samples[i - 1];
			const WeaponTrailComponent::Sample& newer = trail.samples[i];

			shader->SetUniform3f("olderBase", older.base.x, older.base.y, older.base.z);
			shader->SetUniform3f("olderTip",  older.tip.x,  older.tip.y,  older.tip.z);
			shader->SetUniform3f("newerBase", newer.base.x, newer.base.y, newer.base.z);
			shader->SetUniform3f("newerTip",  newer.tip.x,  newer.tip.y,  newer.tip.z);
			shader->SetUniform1f("olderLife", 1.0f - older.age / trail.lifetimeSeconds);
			shader->SetUniform1f("newerLife", 1.0f - newer.age / trail.lifetimeSeconds);

			rendererDevice.DrawElements(*quad);
		}
	}

	rendererDevice.SetDepthWrite(true);
	rendererDevice.SetAdditiveBlending(false);
}

}
