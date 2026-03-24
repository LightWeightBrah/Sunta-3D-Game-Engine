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
};

}