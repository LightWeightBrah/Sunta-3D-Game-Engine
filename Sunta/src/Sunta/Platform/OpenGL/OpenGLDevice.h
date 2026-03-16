#pragma once

#include "Renderer/RendererDevice.h"

namespace Sunta
{

class OpenGLDevice : public RendererDevice
{
public:
	virtual void Clear(float r, float g, float b, float a) override;
	virtual void DrawElements(unsigned int count) override;

	virtual std::shared_ptr<VertexBuffer> CreateVertexBuffer(const BufferDescriptor& descriptor) override;
	virtual std::shared_ptr<ElementBuffer> CreateElementBuffer(const BufferDescriptor& descriptor) override;
	virtual std::shared_ptr<VertexArray> CreateVertexArrayBuffer() override;
};


}