#pragma once

#include <memory>

namespace Sunta
{

class VertexBuffer;
class ElementBuffer;
class VertexArray;
class Texture;
class Shader;

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

// Factory of Renderer Device once per GPU (for basic usage)

class RendererDevice
{
public:
	virtual ~RendererDevice() = default;

	virtual void Clear(float r, float g, float b, float a) = 0;
	virtual void SetViewport(int x, int y, int width, int height) = 0;
	virtual void SetLineWidth(float width) = 0;

	// For drawing 2D over the scene (the game's UI): no depth test, and see-through pixels mixed with what is behind them
	virtual void SetDepthTest(bool isEnabled) = 0;
	virtual void SetAlphaBlending(bool isEnabled) = 0;

	// For light-like effects (embers, sparks): the color is added to what is behind (overlapping ones get brighter),
	// and see-through things that shouldn't hide each other are drawn without writing their depth
	virtual void SetAdditiveBlending(bool isEnabled) = 0;
	virtual void SetDepthWrite(bool isEnabled) = 0;

	virtual void DrawElements(const VertexArray& vertexArray) = 0;
	virtual void DrawLines(const VertexArray& vertexArray) = 0;

	virtual std::shared_ptr<VertexBuffer>  CreateVertexBuffer(const BufferDescriptor& descriptor) = 0;
	virtual std::shared_ptr<ElementBuffer> CreateElementBuffer(const BufferDescriptor& descriptor) = 0;
	virtual std::shared_ptr<VertexArray>   CreateVertexArray() = 0;
	virtual std::shared_ptr<Texture>       CreateTexture(const std::string& filepath, bool flip = true, bool isPixelArt = false) = 0;

	// A texture made in memory (e.g. the letters of a font): 4 bytes per pixel (RGBA), the first row is the top of the picture
	virtual std::shared_ptr<Texture>       CreateTexture(int width, int height, const unsigned char* rgbaPixels) = 0;
	virtual std::shared_ptr<Shader>        CreateShader(const std::string& filepath) = 0;

	static std::unique_ptr<RendererDevice> Create();
};

}