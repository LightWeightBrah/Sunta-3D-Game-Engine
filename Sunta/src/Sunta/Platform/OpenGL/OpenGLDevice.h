#pragma once

#include "Renderer/RendererDevice.h"

namespace Sunta
{

class OpenGLDevice : public RendererDevice
{
public:
	virtual void Clear(float r, float g, float b, float a) override;
	virtual void SetViewport(int x, int y, int width, int height) override;
	virtual void SetLineWidth(float width) override;
	virtual void SetDepthTest(bool isEnabled) override;
	virtual void SetAlphaBlending(bool isEnabled) override;
	virtual void SetAdditiveBlending(bool isEnabled) override;
	virtual void SetDepthWrite(bool isEnabled) override;
	virtual void SetPolygonOffset(bool isEnabled, float factor = 0.0f, float units = 0.0f) override;

	virtual void DrawElements(const VertexArray& vertexArray) override;
	virtual void DrawLines(const VertexArray& vertexArray) override;

	virtual std::shared_ptr<VertexBuffer>  CreateVertexBuffer(const BufferDescriptor& descriptor) override;
	virtual std::shared_ptr<ElementBuffer> CreateElementBuffer(const BufferDescriptor& descriptor) override;
	virtual std::shared_ptr<VertexArray>   CreateVertexArray() override;
	virtual std::shared_ptr<Texture>       CreateTexture(const std::string& filepath, bool flip = true, bool isPixelArt = false) override;
	virtual std::shared_ptr<Texture>       CreateTexture(int width, int height, const unsigned char* rgbaPixels) override;
	virtual std::shared_ptr<Shader>        CreateShader(const std::string& filepath) override;
	virtual std::shared_ptr<Cubemap>       CreateCubemap(const std::string& folder) override;
	virtual std::shared_ptr<Framebuffer>   CreateFramebuffer(const FramebufferSpecification& specification) override;
};


}