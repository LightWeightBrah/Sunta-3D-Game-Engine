#include "SuntaPreCompiled.h"

#include "Renderer.h"

#include "ElementBuffer.h"
#include "VertexArray.h"

#include "Shader.h"
#include "Material.h"

#include "Mesh.h"
#include "Model.h"
#include "Animator.h"

#include "Camera.h"
#include "Log.h"

namespace Sunta
{
	void GLClearError()
	{
		while (glGetError() != GL_NO_ERROR);
	}
	
	bool GLLogCall(const char* function, const char* file, int line)
	{
		while (GLenum error = glGetError())
		{
			SUNTA_ENGINE_LOG_ERROR("OPEN_GL ERROR ({}): {} {}: {}", error, function, file, line);
			return false;
		}
	
		return true;
	}
	
	void Renderer::Clear(float r, float g, float b, float a) const
	{
		GLCall(glClearColor(r, g,b, a));
		GLCall(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));
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