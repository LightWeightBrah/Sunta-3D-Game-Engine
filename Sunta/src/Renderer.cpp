#include <iostream>

#include "Renderer.h"

#include "ElementBuffer.h"
#include "VertexArray.h"

#include "Shader.h"
#include "Material.h"

#include "Mesh.h"
#include "Model.h"
#include "Animator.h"

#include "Camera.h"
#include "Entity.h"
#include "LightSource.h"

void GLClearError()
{
	while (glGetError() != GL_NO_ERROR);
}

bool GLLogCall(const char* function, const char* file, int line)
{
	while (GLenum error = glGetError())
	{
		std::cout << "OPEN_GL ERROR (" << error << "): " << function << " "
			<< file << ":" << line << std::endl;
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

void Renderer::BeginScene(SceneData& data)
{
	sceneData = data;
}

void Renderer::DrawScene(Entity& entity) const
{
	//TODO: CHANGE TO ASSERT 
	if (!entity.GetMesh())
	{
		std::cout << "ERROR: Entity has no mesh" << std::endl;
		return;
	}

	if (!entity.GetMaterial())
	{
		std::cout << "ERROR: Entity has no material" << std::endl;
		return;
	}

	if (!sceneData.lightSource)
	{
		std::cout << "ERROR: SceneData has no lightSource" << std::endl;
		return;
	}
	//TODO : CHANGE TO ASSERT

	DrawEntity(entity);
	DrawLightSource();
}

void Renderer::DrawEntity(Entity& entity) const
{
	auto entityShader = entity.GetMaterial()->GetShader();

	entityShader->Bind();

	entityShader->SetUniformMatrix4fv("model",			entity.GetModelMatrix());
	entityShader->SetUniformMatrix4fv("view",			sceneData.viewMatrix);
	entityShader->SetUniformMatrix4fv("projection",		sceneData.projectionMatrix);
	entityShader->SetUniform3f       ("viewerPosition",	sceneData.cameraPosition);

	sceneData.lightSource->ApplyLight(*entityShader);
	entity.GetMaterial()->ApplyLight();

	DrawMesh(*entity.GetMesh(), *entityShader);
}

void Renderer::DrawLightSource() const
{
	auto light  = sceneData.lightSource;
	auto shader = light->GetShader();

	shader->Bind();

	shader->SetUniformMatrix4fv("model",		light->GetModelMatrix());
	shader->SetUniformMatrix4fv("view",			sceneData.viewMatrix);
	shader->SetUniformMatrix4fv("projection",	sceneData.projectionMatrix);

	DrawMesh(*light->GetMesh(), *shader);
}