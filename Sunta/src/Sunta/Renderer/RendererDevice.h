#pragma once

#include <memory>

namespace Sunta
{

class VertexBuffer;
class ElementBuffer;
class VertexArray;

enum class BufferUsage 
{ 
	None = 0, 
	Static, 
	Dynamic, 
	Stream
};

struct BufferDescriptor
{
	unsigned int	size;
	const void*		data;
	BufferUsage		usage = BufferUsage::Static;
};

// Factory of Renderer Device once per GPU

class RendererDevice
{
public:
	virtual ~RendererDevice() = default;

	virtual void Clear(float r, float g, float b, float a) = 0;
	virtual void SetViewport(int x, int y, int width, int height) = 0;
	virtual void DrawElements(const std::shared_ptr<VertexArray>& VAO) = 0;

	virtual std::shared_ptr<VertexBuffer>  CreateVertexBuffer(const BufferDescriptor& descriptor) = 0;
	virtual std::shared_ptr<ElementBuffer> CreateElementBuffer(const BufferDescriptor& descriptor) = 0;
	virtual std::shared_ptr<VertexArray>   CreateVertexArrayBuffer() = 0;

	static std::unique_ptr<RendererDevice> Create();
};

}