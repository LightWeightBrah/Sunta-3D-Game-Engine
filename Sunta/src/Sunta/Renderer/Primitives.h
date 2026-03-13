#pragma once
#include <memory>

namespace Sunta
{
	class Mesh;
	
	class Primitives
	{
	public:
		static std::unique_ptr<Mesh> CreateCube();
	};
}