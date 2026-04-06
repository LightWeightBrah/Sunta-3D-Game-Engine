#include "Core/SuntaPreCompiled.h"
#include "Renderer.h"

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

#include "Events/EventBus.h"
#include "Events/EventTypes.h"
#include "Core/Assert.h"

namespace Sunta
{

std::unique_ptr<RendererDevice> Renderer::rendererDevice;
	
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
	rendererDevice->Clear(r, g, b, a);
}

void Renderer::DrawMesh(const Mesh& mesh, Material& material, const glm::mat4& modelMatrix, const SceneData& sceneData) const
{
	auto shader = material.GetShader();
	shader->Bind();

	SetBaseTransform(*shader, modelMatrix, sceneData);
	SetBaseLighting(*shader, sceneData);

	material.Apply();
	mesh.Bind();
	rendererDevice->DrawElements(mesh.GetVertexArray());
}
	
void Renderer::DrawModel(const Model& model, const glm::mat4& modelMatrix, const SceneData& sceneData, const Animator* animator) const
{
	bool hasAnimations = (animator && model.HasAnimations());

	for (const auto& subMesh : model.GetSubMeshes())
	{
		if (!subMesh.mesh || !subMesh.material)
			continue;

		auto shader = subMesh.material->GetShader();
		shader->Bind();

		SetBaseTransform(*shader, modelMatrix, sceneData);
		SetBaseLighting(*shader, sceneData);
			
		shader->SetUniform1i("hasAnimations", hasAnimations);
		if (hasAnimations)
			shader->SetBoneMatrices(animator->GetFinalBoneMatrices());

		subMesh.material->Apply();
		subMesh.mesh->Bind();

		rendererDevice->DrawElements(subMesh.mesh->GetVertexArray());
	}
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
	shader.SetUniform3f("viewerPosition", sceneData.cameraPosition);

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