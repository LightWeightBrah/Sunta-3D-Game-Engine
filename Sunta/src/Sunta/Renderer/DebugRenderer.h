#pragma once

#include <array>
#include <memory>
#include "glm/glm.hpp"

#include "Physics/CollisionShapes.h"

namespace Sunta
{

class VertexArray;
class VertexBuffer;
class Shader;
struct SceneData;

class DebugRenderer
{
public:
	static void Init();

	static void DrawBoxWireframe(const std::array<glm::vec3, BOX_CORNER_COUNT>& corners, const glm::vec3& color, const SceneData& sceneData);

	static       bool       GetShowAllGizmos()      { return showAllGizmos;      }
	static const glm::vec3& GetGizmosColor()        { return gizmosColor;        }
	static const glm::vec3& GetGizmosCollideColor() { return gizmosCollideColor; }
	static       float      GetGizmosLineWidth()    { return gizmosLineWidth;    }

	static void SetShowAllGizmos(bool show)                   { showAllGizmos      = show;  }
	static void SetGizmosColor(const glm::vec3& color)        { gizmosColor        = color; }
	static void SetGizmosCollideColor(const glm::vec3& color) { gizmosCollideColor = color; }
	static void SetGizmosLineWidth(float width)               { gizmosLineWidth    = width; }

private:

	static inline std::shared_ptr<VertexArray>  vertexArray;
	static inline std::shared_ptr<VertexBuffer> vertexBuffer;
	static inline std::shared_ptr<Shader>       debugShader;

	static inline bool      showAllGizmos           = true;
	static inline glm::vec3 gizmosColor             = glm::vec3(1.0f, 0.0f, 0.0f);
	static inline glm::vec3 gizmosCollideColor      = glm::vec3(1.0f, 1.0f, 0.0f);
	static inline float     gizmosLineWidth         = 1.0f;

};
}