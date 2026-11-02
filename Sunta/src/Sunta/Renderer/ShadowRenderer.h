#pragma once

#include <memory>
#include <glm/glm.hpp>

namespace Sunta
{

class Animator;
class Mesh;
class Model;
class Shader;
class Framebuffer;
struct SceneData;

// How the shadows of a directional light look (see DirectionalLightComponent)
struct ShadowSettings
{
	// Shadows are drawn this many meters around the camera's surroundings, farther there are none
	float distance   = 30.0f;

	// 0 = no darkening, 1 = the light is fully blocked in a shadow (only the ambient light is left)
	float strength   = 0.85f;

	// How soft the shadow's edge is: 0 = hard, 1 = 3x3 samples, 2 = 5x5, 3 = 7x7 (PCF)
	int   softness   = 1;

	// The width of the shadow map in pixels: sharper shadows for more memory and time
	int   resolution = 2048;
};

// The shadows of the moon or the sun (see learnopengl.com: Shadow Mapping)
//
// Every frame, before the scene is drawn: Begin, then everything that casts shadows with DrawModel and DrawMesh, then End
// The light looks at a square around the camera with an orthographic camera (its rays are parallel) and the depth of
// what it sees goes into the shadow map. The Lit shader then compares each point's distance from the light with
// the map, and averages a few neighbours of the map for a soft edge (PCF)
class ShadowRenderer
{
public:
	// False when there will be no shadows this frame (the map could not be made)
	static bool Begin(const ShadowSettings& settings, const glm::vec3& lightDirection, const SceneData& sceneData);

	static void DrawModel(const Model& model, const glm::mat4& modelMatrix, const Animator* animator);
	static void DrawMesh(const Mesh& mesh, const glm::mat4& modelMatrix);

	// Back to drawing the scene, which now knows where the shadows are
	static void End(SceneData& inOutSceneData);

	// For the Lit shader: the shadow map on this texture slot (above the slots of the materials)
	static void BindShadowMap(unsigned int slot);

private:
	static inline std::shared_ptr<Shader>      depthShader;

	// Only a depth picture: what the light sees
	static inline std::shared_ptr<Framebuffer> shadowMap;

	static inline glm::mat4      lightSpaceMatrix = glm::mat4(1.0f);
	static inline ShadowSettings currentSettings;

	static void EnsureShadowMap(int resolution);
};

}
