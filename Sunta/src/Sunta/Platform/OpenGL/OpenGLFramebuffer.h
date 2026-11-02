#pragma once

#include <vector>

#include "Renderer/Framebuffer.h"

namespace Sunta
{

class OpenGLFramebuffer : public Framebuffer
{
public:
	explicit OpenGLFramebuffer(const FramebufferSpecification& specification);
	virtual ~OpenGLFramebuffer() override;

	virtual void Bind() override;
	virtual void Unbind() override;
	virtual void Clear() override;
	virtual void Resize(int width, int height) override;

	virtual void BindColorTexture(unsigned int index, unsigned int slot) const override;
	virtual void BindDepthTexture(unsigned int slot) const override;

	virtual const FramebufferSpecification& GetSpecification() const override { return specification; }
	virtual bool IsComplete() const override { return isComplete; }

private:
	FramebufferSpecification specification;

	unsigned int              id = 0;
	std::vector<unsigned int> colorTextures;
	unsigned int              depthTexture = 0;
	bool                      isComplete = false;

	// What was drawn into before Bind
	int previousFramebuffer = 0;
	int previousViewport[4] = { 0, 0, 0, 0 };

	void Create();
	void Destroy();
};

}
