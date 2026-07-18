#pragma once

#include "Renderer/RendererDevice.h"

namespace Sunta
{

class OpenGLDevice : public RendererDevice
{
public:
	virtual void Clear(float r, float g, float b, float a) override;
	virtual void SetViewport(int x, int y, int width, int height) override;
	virtual void DrawElements(const VertexArray& vertexArray) override;

	virtual std::shared_ptr<VertexBuffer>  CreateVertexBuffer(const BufferDescriptor& descriptor) override;
	virtual std::shared_ptr<ElementBuffer> CreateElementBuffer(const BufferDescriptor& descriptor) override;
	virtual std::shared_ptr<VertexArray>   CreateVertexArray() override;
	virtual std::shared_ptr<Texture>       CreateTexture(const std::string& filepath, bool flip = true) override;
	virtual std::shared_ptr<Shader>        CreateShader(const std::string& filepath) override;
};


}