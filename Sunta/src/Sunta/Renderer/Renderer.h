#pragma once
#include <glad/glad.h>
#include <vector>

#include "Sunta/Scene/SceneData.h"

namespace Sunta
{

class ElementBuffer;
class VertexArray;

class Texture;
class Shader;
class Material;

class Mesh;
class Model;
class Animator;

class Camera;
class PrimitiveEntity;

class RendererDevice;

void GLClearError();
bool GLLogCall(const char* function, const char* file, int line);

class Renderer
{
public:
	static void Init();
	static void Clear(float r, float g, float b, float a);

	void DrawMesh(const Mesh& mesh, Material& material, const glm::mat4& modelMatrix, const SceneData& sceneData) const;
	void DrawModel(const Model& model, const glm::mat4& modelMatrix, const SceneData& sceneData, const Animator* animator) const;
	void DrawLigthSource(const Mesh& mesh, Shader& shader, const glm::mat4& modelMatrix, const SceneData& sceneData) const;

	static RendererDevice& GetDevice() { return *rendererDevice; }

private:
	// inline static doesn't work well with unique_ptr so we use standard static
	// inline static with unique_ptr would require full definition of RendererDevice 
	// in all files that includes Renderer.h in order to know the destructor of RendererDevice
	static std::unique_ptr<RendererDevice> rendererDevice;
	SceneData sceneData;

	void SetBaseTransform(Shader& shader, const glm::mat4& modelMatrix, const SceneData& sceneData) const;
	void SetBaseLighting(Shader& shader, const SceneData& sceneData) const;

	void SetDirectionalLights(Shader& shader, const SceneData& sceneData) const;
	void SetPointLights(Shader& shader, const SceneData& sceneData) const;
	void SetSpotLights(Shader& shader, const SceneData& sceneData) const;

};

}