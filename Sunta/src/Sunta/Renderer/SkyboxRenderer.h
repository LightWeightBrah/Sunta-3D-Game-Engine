#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <glm/glm.hpp>

namespace Sunta
{

class Cubemap;
class Shader;
class VertexArray;

// Draws the sky: a cube around the camera with a cubemap on its inside (see learnopengl.com: Cubemaps)
//
// It is drawn first, without writing depth, so everything drawn after it covers it: the sky is behind everything
// Cubemaps are loaded the first time their folder is drawn and kept
class SkyboxRenderer
{
public:
	// False when the folder's pictures can't be read (nothing is drawn)
	static bool Draw(const std::string& folder, const glm::mat4& view, const glm::mat4& projection, float brightness);

private:
	static inline std::shared_ptr<VertexArray> cube;
	static inline std::shared_ptr<Shader>      shader;
	static inline std::unordered_map<std::string, std::shared_ptr<Cubemap>> cubemaps;

	static void EnsureInitialized();
	static Cubemap* GetCubemap(const std::string& folder);
};

}
