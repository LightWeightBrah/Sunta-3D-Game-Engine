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

	// The color of the last Clear: the background where nothing is drawn (also of a scene drawn off screen)
	static glm::vec4 GetClearColor() { return clearColor; }

	void DrawMesh(const Mesh& mesh, Material& material, const glm::mat4& modelMatrix, const SceneData& sceneData) const;
	// 'emissiveStrength' scales the model's own light (its emissive maps, 0 = off), 'darkness' takes its colors away
	// (1 = a black silhouette) and 'glowColor' is light added all over it
	// 'parts' draws all of its meshes, or only the solid ones / only the see-through (crystal) ones: a scene draws the
	// solid ones first, copies the picture, then the crystal ones that show it through themselves (see Scene::Render)
	enum class ModelParts { All, Solid, SeeThrough };
	void DrawModel(const Model& model, const glm::mat4& modelMatrix, const SceneData& sceneData, const Animator* animator,
		float emissiveStrength = 1.0f, float darkness = 0.0f, const glm::vec3& glowColor = glm::vec3(0.0f),
		const glm::vec3& rimGlow = glm::vec3(0.0f), float expression = 0.0f, ModelParts parts = ModelParts::All, float fogAmount = 1.0f) const;

	// True when a model has a see-through (crystal) mesh
	static bool HasSeeThroughParts(const Model& model);
	// The aura of a model (see ModelComponent::aura): its copy made bigger along its normals, drawn as added light
	// brightest where it turns away from the camera - a soft glow out beyond its outline. After the solid things
	void DrawModelAura(const Model& model, const glm::mat4& modelMatrix, const SceneData& sceneData, const Animator* animator,
		const glm::vec3& auraColor, float auraSize) const;
	void DrawLigthSource(const Mesh& mesh, Shader& shader, const glm::mat4& modelMatrix, const SceneData& sceneData) const;

	static RendererDevice& GetDevice() { return *rendererDevice; }

private:
	// inline static doesn't work well with unique_ptr so we use standard static
	// inline static with unique_ptr would require full definition of RendererDevice 
	// in all files that includes Renderer.h in order to know the destructor of RendererDevice
	static std::unique_ptr<RendererDevice> rendererDevice;
	static inline glm::vec4 clearColor = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
	SceneData sceneData;

	void SetBaseTransform(Shader& shader, const glm::mat4& modelMatrix, const SceneData& sceneData) const;
	void SetBaseLighting(Shader& shader, const SceneData& sceneData) const;

public:
	// Call when the lights of a picture are set (each render of the scene, each view): the lit shaders send them again
	// once, at their next draw. Sending all of them before every model (26 point lights, dozens of uniforms found by
	// name) cost about a tenth of a millisecond a model
	static void NewLightingVersion() { ++lightingVersion; }
private:
	static inline unsigned int lightingVersion = 1;

	void SetDirectionalLights(Shader& shader, const SceneData& sceneData) const;
	void SetPointLights(Shader& shader, const SceneData& sceneData) const;
	void SetSpotLights(Shader& shader, const SceneData& sceneData) const;

};

}