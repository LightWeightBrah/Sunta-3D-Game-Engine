#pragma once
#include <memory>

namespace Sunta
{

class RendererDevice;

class Mesh;

class Primitives
{
public:
	static std::unique_ptr<Mesh> CreateCube(RendererDevice& rendererDevice);
	static std::unique_ptr<Mesh> CreatePyramide(RendererDevice& rendererDevice);
	static std::unique_ptr<Mesh> CreateSphere(RendererDevice& rendererDevice);
	static std::unique_ptr<Mesh> CreateCone(RendererDevice& rendererDevice);

private:
	static constexpr float PI = 3.14159265359f;
	static constexpr int floatsPerVertex = 8; // 8 => (3 pos + 3 normals + 2 text coords)
};

}