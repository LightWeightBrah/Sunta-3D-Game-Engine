#include "Core/SuntaPreCompiled.h"
#include "Renderer.h"
#include "PostProcessing.h"

#include "ElementBuffer.h"
#include "VertexArray.h"

#include "Shader.h"
#include "Material.h"

#include "Mesh.h"
#include "Model.h"
#include "Animation/Animator.h"

#include "Scene/Camera.h"
#include "Core/Log.h"
#include "RendererDevice.h"
#include "ShadowRenderer.h"

#include "Events/EventBus.h"
#include "Events/EventTypes.h"
#include "Core/Assert.h"

namespace Sunta
{

std::unique_ptr<RendererDevice> Renderer::rendererDevice;

namespace
{

// The shadow map's texture slot: above the slots the materials use for their maps
constexpr unsigned int ShadowMapSlot = 10;
constexpr unsigned int SceneCopySlot = 11; // the scene behind a see-through part (see DrawModel)

}
	
void Renderer::Init()
{
	rendererDevice = RendererDevice::Create();
	SUNTA_ASSERT(rendererDevice, "Renderer: Failed to create Renderer Device!");

	EventBus::Subscribe<WindowResizeEvent>([](const auto& event) 
		{ 
			rendererDevice->SetViewport(0, 0, event.width, event.height);
			SUNTA_ENGINE_LOG_INFO("Renderer: Viewport changed to {0}:{1}", event.width, event.height);
		});


	SUNTA_ENGINE_LOG_INFO("Renderer initalized");
}

void Renderer::Clear(float r, float g, float b, float a)
{
	clearColor = glm::vec4(r, g, b, a);
	rendererDevice->Clear(r, g, b, a);
}

void Renderer::DrawMesh(const Mesh& mesh, Material& material, const glm::mat4& modelMatrix, const SceneData& sceneData) const
{
	auto shader = material.GetShader();
	shader->Bind();

	SetBaseTransform(*shader, modelMatrix, sceneData);
	SetBaseLighting(*shader, sceneData);
	shader->TrySetUniform1f("modelDarkness", 0.0f);
	shader->TrySetUniform1f("modelFogAmount", 1.0f);
	shader->TrySetUniform3f("modelGlow", glm::vec3(0.0f));
	shader->TrySetUniform3f("modelRimGlow", glm::vec3(0.0f));
	shader->TrySetUniform1f("expressionWeight", 0.0f);

	if (shader->HasFeature(ShaderFeature::Skinning))
		shader->SetUniform1i("hasAnimations", false);

	material.Apply();
	mesh.Bind();
	rendererDevice->DrawElements(mesh.GetVertexArray());
}
	
bool Renderer::HasSeeThroughParts(const Model& model)
{
	for (const auto& subMesh : model.GetSubMeshes())
		if (subMesh.material && subMesh.material->GetData().crystal > 0.0f)
			return true;
	return false;
}

void Renderer::DrawModel(const Model& model, const glm::mat4& modelMatrix, const SceneData& sceneData, const Animator* animator,
	float emissiveStrength, float darkness, const glm::vec3& glowColor, const glm::vec3& rimGlow, float expression, ModelParts parts,
	float fogAmount) const
{
	for (const auto& subMesh : model.GetSubMeshes())
	{
		if (!subMesh.mesh || !subMesh.material)
			continue;

		const bool isSeeThrough = subMesh.material->GetData().crystal > 0.0f;
		if ((parts == ModelParts::Solid && isSeeThrough) || (parts == ModelParts::SeeThrough && !isSeeThrough))
			continue;

		auto shader = subMesh.material->GetShader();
		shader->Bind();

		SetBaseTransform(*shader, modelMatrix, sceneData);
		SetBaseLighting(*shader, sceneData);
		shader->TrySetUniform1f("emissiveStrength", emissiveStrength);
		shader->TrySetUniform1f("modelDarkness", darkness);
		shader->TrySetUniform1f("modelFogAmount", fogAmount);
		shader->TrySetUniform3f("modelGlow", glowColor);
		shader->TrySetUniform3f("modelRimGlow", rimGlow);
		shader->TrySetUniform1f("expressionWeight", expression);

		// What is behind a see-through part: the copy of the scene drawn so far (none: it is milky, as without it)
		const bool showsSceneThrough = parts == ModelParts::SeeThrough && isSeeThrough && PostProcessing::HasSceneCopy();
		shader->TrySetUniformBool("hasSceneCopy", showsSceneThrough);
		if (showsSceneThrough)
		{
			PostProcessing::BindSceneCopy(SceneCopySlot);
			shader->TrySetUniform1i("sceneCopy", SceneCopySlot);
			const glm::vec2 copySize = PostProcessing::GetSceneCopySize();
			shader->TrySetUniform4f("sceneCopySize", copySize.x, copySize.y, 0.0f, 0.0f);
		}
			
		bool supportsSkinning = shader->HasFeature(ShaderFeature::Skinning);
		bool isSkinnedMesh = supportsSkinning && model.HasBones();

		if (supportsSkinning)
		{
			shader->SetUniform1i("hasAnimations", isSkinnedMesh);

			if (isSkinnedMesh)
			{
				if(animator && animator->GetCurrentAnimation())
					shader->TrySetBoneMatrices(animator->GetFinalBoneMatrices());
				else
					shader->TrySetBoneMatrices(model.GetDefaultBoneMatrices());
			}
		}

		subMesh.material->Apply();
		subMesh.mesh->Bind();

		rendererDevice->DrawElements(subMesh.mesh->GetVertexArray());
	}
}

void Renderer::DrawModelAura(const Model& model, const glm::mat4& modelMatrix, const SceneData& sceneData, const Animator* animator,
	const glm::vec3& auraColor, float auraSize) const
{
	rendererDevice->SetAdditiveBlending(true);
	rendererDevice->SetDepthWrite(false);
	// (from the inside: the figure hides the copy in front of it, only the glow around its outline is seen)
	rendererDevice->SetBackFacesOnly(true);

	for (const auto& subMesh : model.GetSubMeshes())
	{
		if (!subMesh.mesh || !subMesh.material)
			continue;

		auto shader = subMesh.material->GetShader();
		shader->Bind();
		SetBaseTransform(*shader, modelMatrix, sceneData);
		SetBaseLighting(*shader, sceneData);
		shader->TrySetUniform1f("expressionWeight", 0.0f);
		shader->TrySetUniform1f("auraInflate", auraSize);
		shader->TrySetUniform3f("modelAura", auraColor);

		const bool isSkinnedMesh = shader->HasFeature(ShaderFeature::Skinning) && model.HasBones();
		if (shader->HasFeature(ShaderFeature::Skinning))
		{
			shader->SetUniform1i("hasAnimations", isSkinnedMesh);
			if (isSkinnedMesh)
				shader->TrySetBoneMatrices(animator && animator->GetCurrentAnimation() ? animator->GetFinalBoneMatrices()
				                                                                         : model.GetDefaultBoneMatrices());
		}

		subMesh.material->Apply();
		subMesh.mesh->Bind();
		rendererDevice->DrawElements(subMesh.mesh->GetVertexArray());

		// (back to an ordinary draw for the next model that uses this shader)
		shader->TrySetUniform1f("auraInflate", 0.0f);
	}

	rendererDevice->SetBackFacesOnly(false);
	rendererDevice->SetDepthWrite(true);
	rendererDevice->SetAdditiveBlending(false);
}

void Renderer::DrawLigthSource(const Mesh& mesh, Shader& shader, const glm::mat4& modelMatrix, const SceneData& sceneData) const
{
	shader.Bind();
	SetBaseTransform(shader, modelMatrix, sceneData);

	mesh.Bind();

	rendererDevice->DrawElements(mesh.GetVertexArray());
}

void Renderer::SetBaseTransform(Shader& shader, const glm::mat4& modelMatrix, const SceneData& sceneData) const
{
	shader.SetUniformMatrix4fv("model",		 modelMatrix);
	shader.SetUniformMatrix4fv("view",		 sceneData.viewMatrix);
	shader.SetUniformMatrix4fv("projection", sceneData.projectionMatrix);
}

void Renderer::SetBaseLighting(Shader& shader, const SceneData& sceneData) const
{
	if (!shader.HasFeature(ShaderFeature::Lighting))
		return;

	shader.SetUniform3f("viewerPosition", sceneData.cameraPosition);
	shader.TrySetUniform3f("fogColor", sceneData.fogColor);
	shader.TrySetUniform1f("fogDensity", sceneData.fogDensity);

	shader.TrySetUniformBool("hasShadows", sceneData.hasShadows);
	if (sceneData.hasShadows)
	{
		ShadowRenderer::BindShadowMap(ShadowMapSlot);
		shader.TrySetUniform1i("shadowMap", ShadowMapSlot);
		shader.TrySetUniformMatrix4fv("lightSpaceMatrix", sceneData.lightSpaceMatrix);
		shader.TrySetUniform1f("shadowStrength", sceneData.shadowStrength);
		shader.TrySetUniform1f("shadowNormalOffset", sceneData.shadowNormalOffset);
		shader.TrySetUniform1i("shadowSoftness", sceneData.shadowSoftness);
	}

	// The lights: once a picture per shader (see NewLightingVersion)
	if (shader.uploadedLightingVersion == lightingVersion)
		return;
	shader.uploadedLightingVersion = lightingVersion;
	SetDirectionalLights(shader, sceneData);
	SetPointLights(shader, sceneData);
	SetSpotLights(shader, sceneData);
}

void Renderer::SetDirectionalLights(Shader& shader, const SceneData& sceneData) const
{
	shader.SetUniform1i("directionalLightsCount", (int)sceneData.directionalLights.size());

	for (unsigned int i = 0; i < sceneData.directionalLights.size(); i++)
	{
		std::string base = "directionalLights[" + std::to_string(i) + "].";

		shader.SetUniform3f(base + "direction",				  sceneData.directionalLights[i].direction);

		shader.SetUniform3f(base + "color.ambientIntensity",  sceneData.directionalLights[i].color.ambientIntensity);
		shader.SetUniform3f(base + "color.diffuseIntensity",  sceneData.directionalLights[i].color.diffuseIntensity);
		shader.SetUniform3f(base + "color.specularIntensity", sceneData.directionalLights[i].color.specularIntensity);
	}
}

void Renderer::SetPointLights(Shader& shader, const SceneData& sceneData) const
{
	shader.SetUniform1i("pointLightsCount", (int)sceneData.pointLights.size());

	for (unsigned int i = 0; i < sceneData.pointLights.size(); i++)
	{
		std::string base = "pointLights[" + std::to_string(i) + "].";

		shader.SetUniform3f(base + "position",				  sceneData.pointLights[i].position);

		shader.SetUniform3f(base + "color.ambientIntensity",  sceneData.pointLights[i].color.ambientIntensity);
		shader.SetUniform3f(base + "color.diffuseIntensity",  sceneData.pointLights[i].color.diffuseIntensity);
		shader.SetUniform3f(base + "color.specularIntensity", sceneData.pointLights[i].color.specularIntensity);

		shader.SetUniform1f(base + "attenuation.constant",    sceneData.pointLights[i].attenuation.constant);
		shader.SetUniform1f(base + "attenuation.linear",      sceneData.pointLights[i].attenuation.linear);
		shader.SetUniform1f(base + "attenuation.quadratic",   sceneData.pointLights[i].attenuation.quadratic);
	}
}

void Renderer::SetSpotLights(Shader& shader, const SceneData& sceneData) const
{
	shader.SetUniform1i("spotLightsCount", (int)sceneData.spotlights.size());

	for (unsigned int i = 0; i < sceneData.spotlights.size(); i++)
	{
		std::string base = "spotLights[" + std::to_string(i) + "].";

		shader.SetUniform3f(base + "position",				  sceneData.spotlights[i].position);
		shader.SetUniform3f(base + "spotlightDirection",	  sceneData.spotlights[i].spotlightDirection);

		shader.SetUniform1f(base + "innerCutOffAngle",		  glm::cos(glm::radians(sceneData.spotlights[i].innercutOffAngle)));
		shader.SetUniform1f(base + "outerCutOffAngle",		  glm::cos(glm::radians(sceneData.spotlights[i].outerCutOffAngle)));

		shader.SetUniform3f(base + "color.ambientIntensity",  sceneData.spotlights[i].color.ambientIntensity);
		shader.SetUniform3f(base + "color.diffuseIntensity",  sceneData.spotlights[i].color.diffuseIntensity);
		shader.SetUniform3f(base + "color.specularIntensity", sceneData.spotlights[i].color.specularIntensity);

		shader.SetUniform1f(base + "attenuation.constant",    sceneData.spotlights[i].attenuation.constant);
		shader.SetUniform1f(base + "attenuation.linear",      sceneData.spotlights[i].attenuation.linear);
		shader.SetUniform1f(base + "attenuation.quadratic",   sceneData.spotlights[i].attenuation.quadratic);
	}
}

}