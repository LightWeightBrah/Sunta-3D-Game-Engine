#pragma once

#include "VertexBuffer.h"
#include <memory>

namespace Sunta
{

struct BufferDescriptor
{
	unsigned int	size;
	const void*		data;
};

class RendererDevice
{
public:
	virtual ~RendererDevice() = default;

	virtual void Clear(float r, float g, float b, float a) = 0;
	virtual void DrawElements(unsigned int count) = 0;

	virtual std::shared_ptr<VertexBuffer> CreateVertexBuffer(const BufferDescriptor& descriptor) = 0;

	static std::unique_ptr<RendererDevice> Create();
};

}