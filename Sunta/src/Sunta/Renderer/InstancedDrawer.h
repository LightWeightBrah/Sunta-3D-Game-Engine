#pragma once

#include <memory>

#include "BufferLayout.h"

namespace Sunta
{

class ElementBuffer;
class VertexArray;
class VertexBuffer;

// One shape drawn many times in one call (instancing): thousands of specks of fire, a cloud of missiles
//
// The shape's vertices and triangles are given once; each Draw uploads the data of every copy (one block of
// 'instanceLayout' per copy, its elements marked 'instanced') and draws them all. The buffer of copies grows
// when more are drawn than ever before
class InstancedDrawer
{
public:
	InstancedDrawer(std::shared_ptr<VertexBuffer> vertices, std::shared_ptr<ElementBuffer> triangles, const BufferLayout& instanceLayout);

	// 'data' holds 'instanceCount' blocks of the instance layout's stride. The shader must be bound
	void Draw(const void* data, unsigned int instanceCount);

private:
	std::shared_ptr<VertexBuffer>  vertices;
	std::shared_ptr<ElementBuffer> triangles;
	BufferLayout                   instanceLayout;

	std::shared_ptr<VertexArray>   vertexArray;
	std::shared_ptr<VertexBuffer>  instances;
	unsigned int                   capacity = 0;

	void Grow(unsigned int instanceCount);
};

}
