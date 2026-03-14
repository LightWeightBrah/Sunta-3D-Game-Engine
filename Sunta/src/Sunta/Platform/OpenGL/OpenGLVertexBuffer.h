#pragma once

#include "Renderer/VertexBuffer.h"

namespace Sunta
{

struct BufferDescriptor;

class OpenGLVertexBuffer : public VertexBuffer
{
private:
	unsigned int id;
public:
	OpenGLVertexBuffer(const BufferDescriptor& descriptor);
	virtual ~OpenGLVertexBuffer() override;

	virtual void Bind() const override;
	virtual void Unbind() const override;
};

}