#include "Core/SuntaPreCompiled.h"
#include "LiquidOrbRenderer.h"

#include "ECS/Component.h"
#include "ECS/EntityManager.h"
#include "Core/VirtualFileSystem.h"
#include "Renderer/Mesh.h"
#include "Renderer/Primitives.h"
#include "Renderer/Renderer.h"
#include "Renderer/RendererDevice.h"
#include "Renderer/Shader.h"
#include "Renderer/SkyboxRenderer.h"

namespace Sunta
{

namespace
{

constexpr const char* ShaderPath = "@engine/Shaders/Liquid.shader";

// The texture unit the sky is read from
constexpr int SkySlot = 0;

}

void LiquidOrbRenderer::EnsureInitialized()
{
	if (shader)
		return;

	RendererDevice& rendererDevice = Renderer::GetDevice();

	sphere = Primitives::CreateSphere(rendererDevice);
	shader = rendererDevice.CreateShader(VirtualFileSystem::Resolve(ShaderPath));
}

void LiquidOrbRenderer::Draw(EntityManager& entityManager, const glm::mat4& view, const glm::mat4& projection,
	const glm::vec3& cameraPosition, const std::string& skyboxFolder, float skyBrightness, float seconds)
{
	auto& orbs = entityManager.GetAllComponents<LiquidOrbComponent>();

	const bool hasVisibleOrb = std::any_of(orbs.begin(), orbs.end(), [](const LiquidOrbComponent& orb) { return orb.isVisible; });
	if (!hasVisibleOrb)
		return;

	EnsureInitialized();

	RendererDevice& rendererDevice = Renderer::GetDevice();

	shader->Bind();
	shader->SetUniformMatrix4fv("view", view);
	shader->SetUniformMatrix4fv("projection", projection);
	shader->SetUniform3f("cameraPosition", cameraPosition);
	shader->SetUniform1f("seconds", seconds);
	shader->SetUniform1f("skyBrightness", skyBrightness);

	// Without a sky the orb still has its rim and its highlights
	const bool hasSky = SkyboxRenderer::TryBindSky(skyboxFolder, SkySlot);
	shader->SetUniformBool("hasSky", hasSky);
	shader->SetUniform1i("sky", SkySlot);

	// See-through: blended over what was drawn before it
	rendererDevice.SetAdditiveBlending(false);
	rendererDevice.SetAlphaBlending(true);

	sphere->Bind();

	for (const LiquidOrbComponent& orb : orbs)
	{
		if (!orb.isVisible)
			continue;

		const WorldMatrixComponent* worldMatrix = entityManager.GetComponent<WorldMatrixComponent>(entityManager.GetEntityIDForComponent(orb));
		if (!worldMatrix)
			continue;

		shader->SetUniformMatrix4fv("model", worldMatrix->matrix);
		shader->SetUniform3f("tint", orb.tint);
		shader->SetUniform1f("brightness", orb.brightness);
		shader->SetUniform1f("wobble", orb.wobble);
		shader->SetUniform1f("wobbleSpeed", orb.wobbleSpeed);
		shader->SetUniform1f("clearness", orb.clearness);

		rendererDevice.DrawElements(sphere->GetVertexArray());
	}

	rendererDevice.SetAlphaBlending(false);
}

}
