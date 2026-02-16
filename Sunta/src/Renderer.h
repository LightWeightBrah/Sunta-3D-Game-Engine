#pragma once
#include <GL/glew.h>
#include "SceneData.h"

class ElementBuffer;
class VertexArray;

class Texture;
class Shader;

class Mesh;
class Model;
class Animator;

class Camera;
class Entity;

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
	void Clear(float r, float g, float b, float a) const;
	
	
	void Draw(const VertexArray& VAO, const ElementBuffer& EBO, const Shader& shader) const;
	void DrawMesh(const Mesh& mesh, Shader& shader) const;
	void DrawModel(const Model& model, Shader& shader, const Animator* animator) const;
	
	void BeginScene(SceneData& data);
	void DrawScene(Entity& entity) const;

private:
	SceneData sceneData;

	void DrawEntity(Entity& entity) const;
	void DrawLightSource() const;
};