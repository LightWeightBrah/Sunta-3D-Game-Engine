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

//TEMP TODO: remove all opengl
#include <Platform/OpenGL/OpenGLUtilities.h>

namespace Sunta
{

std::unique_ptr<RendererDevice> Renderer::rendererDevice;
	
void Renderer::Init()
{
	rendererDevice = RendererDevice::Create();

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
	GLCall(glDrawElements(GL_TRIANGLES, mesh.GetIndexCount(), GL_UNSIGNED_INT, 0));
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

		GLCall(glDrawElements(GL_TRIANGLES, subMesh.mesh->GetIndexCount(), GL_UNSIGNED_INT, 0));
	}
}

void Renderer::DrawLigthSource(const Mesh& mesh, Shader& shader, const glm::mat4& modelMatrix, const SceneData& sceneData) const
{
	shader.Bind();
	SetBaseTransform(shader, modelMatrix, sceneData);

	mesh.Bind();
	GLCall(glDrawElements(GL_TRIANGLES, mesh.GetIndexCount(), GL_UNSIGNED_INT, 0));
}

void Renderer::SetBaseTransform(Shader& shader, const glm::mat4& modelMatrix, const SceneData& sceneData) const
{
	shader.SetUniformMatrix4fv("model",		 modelMatrix);
	shader.SetUniformMatrix4fv("view",		 sceneData.viewMatrix);
	shader.SetUniformMatrix4fv("projection", sceneData.projectionMatrix);
}

void Renderer::SetBaseLighting(Shader& shader, const SceneData& sceneData) const
{
	shader.SetUniform3f("viewerPosition",				 sceneData.cameraPosition);

	shader.SetUniform3f("lightSource.position",			 sceneData.lightSourceData.position);
	shader.SetUniform3f("lightSource.ambientIntensity",  sceneData.lightSourceData.ambientIntensity);
	shader.SetUniform3f("lightSource.diffuseIntensity",  sceneData.lightSourceData.diffuseIntensity);
	shader.SetUniform3f("lightSource.specularIntensity", sceneData.lightSourceData.specularIntensity);
}

}