#include "Core/SuntaPreCompiled.h"
#include "InstancedDrawer.h"

#include <glm/glm.hpp>

#include "Renderer.h"
#include "RendererDevice.h"
#include "ScopedVertexArrayBind.h"
#include "VertexArray.h"
#include "VertexBuffer.h"

namespace Sunta
{

namespace
{

constexpr unsigned int SmallestCapacity = 256;

}

InstancedDrawer::InstancedDrawer(std::shared_ptr<VertexBuffer> vertices, std::shared_ptr<ElementBuffer> triangles, const BufferLayout& instanceLayout)
	: vertices(std::move(vertices))
	, triangles(std::move(triangles))
	, instanceLayout(instanceLayout)
{
}

void InstancedDrawer::Grow(unsigned int instanceCount)
{
	// Doubled, so that a growing crowd rebuilds the buffer a few times only
	capacity = glm::max(capacity * 2, glm::max(instanceCount, SmallestCapacity));

	RendererDevice& rendererDevice = Renderer::GetDevice();

	BufferDescriptor descriptor;
	descriptor.data  = nullptr;
	descriptor.size  = capacity * instanceLayout.GetStride();
	descriptor.usage = BufferUsage::Dynamic;
	instances = rendererDevice.CreateVertexBuffer(descriptor);
	instances->SetLayout(instanceLayout);

	// A new vertex array: the shape's vertices first (attributes from 0), the copies after them
	vertexArray = rendererDevice.CreateVertexArray();
	ScopedVertexArrayBind vertexArrayGuard(*vertexArray);
	vertexArray->AddVertexBuffer(vertices);
	vertexArray->AddVertexBuffer(instances);
	vertexArray->SetElementBuffer(triangles);
}

void InstancedDrawer::Draw(const void* data, unsigned int instanceCount)
{
	if (instanceCount == 0)
		return;

	if (instanceCount > capacity)
		Grow(instanceCount);

	instances->UpdateDynamicData(data, instanceCount * instanceLayout.GetStride());
	Renderer::GetDevice().DrawElementsInstanced(*vertexArray, instanceCount);
}

}
