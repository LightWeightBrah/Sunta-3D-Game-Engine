#include "Core/SuntaPreCompiled.h"
#include "ShadowRenderer.h"

#include <glm/gtc/matrix_transform.hpp>

#include "Mesh.h"
#include "Model.h"
#include "Renderer.h"
#include "RendererDevice.h"
#include "Shader.h"
#include "Framebuffer.h"
#include "VertexArray.h"
#include "Animation/Animator.h"
#include "Core/VirtualFileSystem.h"
#include "Scene/SceneData.h"

namespace Sunta
{

namespace
{

constexpr const char* DepthShaderPath = "@engine/Shaders/ShadowDepth.shader";

// The square around the camera reaches this part of the distance ahead of it (the camera of a third person game
// looks over the character's shoulder, so most of what matters is in front)
constexpr float CenterAheadOfCamera = 0.35f;

// Pushes the depth drawn into the map away from the light, so a surface doesn't shadow itself ("shadow acne" stripes)
constexpr float PolygonOffsetFactor = 2.0f;
constexpr float PolygonOffsetUnits  = 4.0f;

// Things this far towards the light from the square still cast shadows into it (tall walls, towers), and this far
// behind it still receive them
constexpr float DepthTowardsLight = 60.0f;
constexpr float DepthAwayFromLight = 20.0f;

// The point is moved this many shadow-map pixels along its normal before the map is read (no self-shadowing stripes)
constexpr float NormalOffsetInTexels = 1.5f;

glm::vec3 GetHorizontalForward(const glm::mat4& view)
{
	// The camera looks along -Z of its view: the third row of the view matrix, negated
	glm::vec3 forward = -glm::vec3(view[0][2], view[1][2], view[2][2]);
	forward.y = 0.0f;

	const float length = glm::length(forward);
	return length > 0.001f ? forward / length : glm::vec3(0.0f, 0.0f, -1.0f);
}

// An orthographic camera along the light's direction, looking at a square of 2 x distance around the center
// The square moves only by whole pixels of the map, so the shadows' edges don't shimmer while the camera moves
glm::mat4 MakeLightSpaceMatrix(const glm::vec3& lightDirection, const glm::vec3& center, float distance, int resolution)
{
	const glm::vec3 direction = glm::normalize(lightDirection);
	const glm::vec3 up = glm::abs(direction.y) > 0.99f ? glm::vec3(0.0f, 0.0f, 1.0f) : glm::vec3(0.0f, 1.0f, 0.0f);

	// Turned towards the light, standing at the origin: the square's place is chosen in the light's own axes
	const glm::mat4 lightView = glm::lookAt(glm::vec3(0.0f), direction, up);

	glm::vec3 centerInLight = glm::vec3(lightView * glm::vec4(center, 1.0f));

	const float texelSize = 2.0f * distance / (float)resolution;
	centerInLight.x = glm::floor(centerInLight.x / texelSize) * texelSize;
	centerInLight.y = glm::floor(centerInLight.y / texelSize) * texelSize;

	// In front of the light is -Z: the center's depth is -z
	const float centerDepth = -centerInLight.z;

	const glm::mat4 lightProjection = glm::ortho(
		centerInLight.x - distance, centerInLight.x + distance,
		centerInLight.y - distance, centerInLight.y + distance,
		centerDepth - distance - DepthTowardsLight, centerDepth + distance + DepthAwayFromLight);

	return lightProjection * lightView;
}

}

void ShadowRenderer::EnsureShadowMap(int resolution)
{
	if (!depthShader)
		depthShader = Renderer::GetDevice().CreateShader(VirtualFileSystem::Resolve(DepthShaderPath));

	if (!shadowMap)
	{
		FramebufferSpecification specification;
		specification.width       = resolution;
		specification.height      = resolution;
		specification.attachments = { FramebufferAttachment::Depth24 };

		shadowMap = Renderer::GetDevice().CreateFramebuffer(specification);
	}
	else
		shadowMap->Resize(resolution, resolution);
}

bool ShadowRenderer::Begin(const ShadowSettings& settings, const glm::vec3& lightDirection, const SceneData& sceneData)
{
	EnsureShadowMap(glm::clamp(settings.resolution, 256, 8192));
	if (!depthShader || !shadowMap || !shadowMap->IsComplete())
		return false;

	currentSettings = settings;

	const glm::vec3 center = sceneData.cameraPosition + GetHorizontalForward(sceneData.viewMatrix) * settings.distance * CenterAheadOfCamera;
	const int resolution = shadowMap->GetSpecification().width;
	lightSpaceMatrix = MakeLightSpaceMatrix(lightDirection, center, settings.distance, resolution);

	// Ended in End: the scene is drawn into the map until then
	shadowMap->Bind();
	shadowMap->Clear();
	Renderer::GetDevice().SetPolygonOffset(true, PolygonOffsetFactor, PolygonOffsetUnits);

	depthShader->Bind();
	depthShader->SetUniformMatrix4fv("lightSpaceMatrix", lightSpaceMatrix);
	return true;
}

void ShadowRenderer::DrawModel(const Model& model, const glm::mat4& modelMatrix, const Animator* animator)
{
	depthShader->SetUniformMatrix4fv("model", modelMatrix);

	const bool isSkinned = model.HasBones();
	depthShader->SetUniform1i("hasAnimations", isSkinned);

	if (isSkinned)
	{
		if (animator && animator->GetCurrentAnimation())
			depthShader->TrySetBoneMatrices(animator->GetFinalBoneMatrices());
		else
			depthShader->TrySetBoneMatrices(model.GetDefaultBoneMatrices());
	}

	for (const auto& subMesh : model.GetSubMeshes())
	{
		if (!subMesh.mesh)
			continue;

		subMesh.mesh->Bind();
		Renderer::GetDevice().DrawElements(subMesh.mesh->GetVertexArray());
	}
}

void ShadowRenderer::DrawMesh(const Mesh& mesh, const glm::mat4& modelMatrix)
{
	depthShader->SetUniformMatrix4fv("model", modelMatrix);
	depthShader->SetUniform1i("hasAnimations", false);

	mesh.Bind();
	Renderer::GetDevice().DrawElements(mesh.GetVertexArray());
}

void ShadowRenderer::End(SceneData& inOutSceneData)
{
	Renderer::GetDevice().SetPolygonOffset(false);
	shadowMap->Unbind();

	inOutSceneData.hasShadows         = true;
	inOutSceneData.lightSpaceMatrix   = lightSpaceMatrix;
	inOutSceneData.shadowStrength     = glm::clamp(currentSettings.strength, 0.0f, 1.0f);
	inOutSceneData.shadowSoftness     = glm::clamp(currentSettings.softness, 0, 3);
	inOutSceneData.shadowNormalOffset = NormalOffsetInTexels * 2.0f * currentSettings.distance / (float)shadowMap->GetSpecification().width;
}

void ShadowRenderer::BindShadowMap(unsigned int slot)
{
	if (shadowMap)
		shadowMap->BindDepthTexture(slot);
}

}
