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

#define ASSERT(x) if (!(x)) __debugbreak();

#ifdef _DEBUG
#define GLCall(x) GLClearError(); x; ASSERT(GLLogCall(#x, __FILE__, __LINE__))
#else
#define GLCall(x) x
#endif


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
	static std::unique_ptr<RendererDevice> rendererDevice;
	SceneData sceneData;

	void SetBaseTransform(Shader& shader, const glm::mat4& modelMatrix, const SceneData& sceneData) const;
	void SetBaseLighting(Shader& shader, const SceneData& sceneData) const;
};

}