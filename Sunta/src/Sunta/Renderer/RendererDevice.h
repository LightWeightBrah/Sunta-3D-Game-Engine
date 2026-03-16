#pragma once

#include <memory>

class VertexBuffer;
class ElementBuffer;
class VertexArray;

namespace Sunta
{

enum class BufferUsage { Static, Dynamic, Stream};

struct BufferDescriptor
{
	unsigned int	size;
	const void*		data;
	BufferUsage		usage = BufferUsage::Static;
};

class RendererDevice
{
public:
	virtual ~RendererDevice() = default;

	virtual void Clear(float r, float g, float b, float a) = 0;
	virtual void DrawElements(unsigned int count) = 0;

	virtual std::shared_ptr<VertexBuffer> CreateVertexBuffer(const BufferDescriptor& descriptor) = 0;
	virtual std::shared_ptr<ElementBuffer> CreateElementBuffer(const BufferDescriptor& descriptor) = 0;
	virtual std::shared_ptr<VertexArray> CreateVertexArrayBuffer() = 0;

	static std::unique_ptr<RendererDevice> Create();
};

}