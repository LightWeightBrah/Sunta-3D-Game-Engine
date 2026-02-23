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
	
	void Renderer::Draw(const VertexArray& VAO, const ElementBuffer& EBO, const Shader& shader) const
	{
		shader.Bind();
		VAO.Bind();
		EBO.Bind();
		GLCall(glDrawElements(GL_TRIANGLES, EBO.GetCount(), GL_UNSIGNED_INT, 0));
	}
	
	void Renderer::DrawModel(const Model& model, Shader& shader, const Animator* animator) const
	{
		shader.Bind();
	
		bool hasAnimations = (animator && model.HasAnimations());
		shader.SetUniform1i("hasAnimations", hasAnimations);
		if (hasAnimations)
			shader.SetBoneMatrices(animator->GetFinalBoneMatrices());
	
		for (const auto& mesh : model.GetMeshes())
			DrawMesh(mesh, shader);
	}
	
	void Renderer::DrawMesh(const Mesh& mesh, Shader& shader) const
	{
		mesh.BindTextures(shader);
		Draw(mesh.GetVAO(), mesh.GetEBO(), shader);
	}
	
	void Renderer::DrawMesh(const Mesh& mesh, Material& material, const glm::mat4& modelMatrix, const SceneData& sceneData) const
	{
		auto shader = material.GetShader();
		shader->Bind();

		shader->SetUniformMatrix4fv("model",		modelMatrix);
		shader->SetUniformMatrix4fv("view",			sceneData.viewMatrix);
		shader->SetUniformMatrix4fv("projection",   sceneData.projectionMatrix);
		shader->SetUniform3f("viewerPosition",		sceneData.cameraPosition);

		shader->SetUniform3f("lightSource.position",			 sceneData.lightSourceData.position);
		shader->SetUniform3f("lightSource.ambientIntensity",  sceneData.lightSourceData.ambientIntensity);
		shader->SetUniform3f("lightSource.diffuseIntensity",  sceneData.lightSourceData.diffuseIntensity);
		shader->SetUniform3f("lightSource.specularIntensity", sceneData.lightSourceData.specularIntensity);

		material.ApplyLight();


		DrawMesh(mesh, *shader);
	}

	void Renderer::DrawLigthSource(const Mesh& mesh, Shader& shader, const glm::mat4& modelMatrix, const SceneData& sceneData) const
	{
		shader.Bind();

		shader.SetUniformMatrix4fv("model",		 modelMatrix);
		shader.SetUniformMatrix4fv("view",		 sceneData.viewMatrix);
		shader.SetUniformMatrix4fv("projection", sceneData.projectionMatrix);

		DrawMesh(mesh, shader);
	}
}