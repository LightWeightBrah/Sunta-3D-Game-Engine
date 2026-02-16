#pragma once
#include <memory>

class Mesh;

class Primitives
{
public:
	static std::unique_ptr<Mesh> CreateCube();
};