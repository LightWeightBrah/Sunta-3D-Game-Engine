#pragma once

#include <vector>

namespace Sunta
{

// What one picture of a framebuffer keeps per pixel
enum class FramebufferAttachment
{
	RGBA8,   // a color, 8 bits per channel (0..1)
	RGBA16F, // a color as floating point numbers, brighter than 1 is kept (for HDR)
	Depth24  // how far the nearest surface is (a shadow map, the depth of a scene drawn off screen)
};

struct FramebufferSpecification
{
	int width  = 1;
	int height = 1;

	// The color pictures in their order (the shader's outputs 0, 1...), at most one depth picture
	std::vector<FramebufferAttachment> attachments;
};

// Somewhere to draw other than the screen (see learnopengl.com: Framebuffers)
//
// The scene drawn into it ends up in its textures, which shaders can read afterwards: the depth seen from a light
// is a shadow map, a scene in floating point colors can be tone mapped or blurred (HDR, bloom)
// Draw into it between Bind and Unbind, or inside a ScopedFramebufferBind (it unbinds at the end of the scope):
// Unbind brings back the framebuffer and the viewport used before (the editor draws the scene in a part of its window)
class Framebuffer
{
public:
	virtual ~Framebuffer() = default;

	// Draws into this framebuffer from now on, with a viewport of its whole size
	virtual void Bind() = 0;

	// Back to the framebuffer and the viewport used before Bind
	virtual void Unbind() = 0;

	// Clears the color pictures to this color (black by default) and the depth to the farthest
	virtual void Clear(float r = 0.0f, float g = 0.0f, float b = 0.0f, float a = 1.0f) = 0;

	// New pictures of this size (the old content is lost)
	virtual void Resize(int width, int height) = 0;

	// For the shaders that read it: the color picture 'index' (in the order of the specification), or the depth
	virtual void BindColorTexture(unsigned int index, unsigned int slot) const = 0;
	virtual void BindDepthTexture(unsigned int slot) const = 0;

	virtual const FramebufferSpecification& GetSpecification() const = 0;

	// False when the graphics card could not make it with these pictures
	virtual bool IsComplete() const = 0;
};

class ScopedFramebufferBind
{
public:
	explicit ScopedFramebufferBind(Framebuffer& framebuffer)
		: framebuffer(framebuffer)
	{
		framebuffer.Bind();
	}

	~ScopedFramebufferBind()
	{
		framebuffer.Unbind();
	}

	ScopedFramebufferBind(const ScopedFramebufferBind&)            = delete;
	ScopedFramebufferBind& operator=(const ScopedFramebufferBind&) = delete;

private:
	Framebuffer& framebuffer;
};

}
