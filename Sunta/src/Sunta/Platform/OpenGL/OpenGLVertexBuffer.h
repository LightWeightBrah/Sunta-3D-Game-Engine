#pragma once

#include "Renderer/VertexBuffer.h"
#include "Renderer/BufferLayout.h"

namespace Sunta
{

struct BufferDescriptor;

class OpenGLVertexBuffer : public VertexBuffer
{
public:
	OpenGLVertexBuffer(const BufferDescriptor& descriptor);
	virtual ~OpenGLVertexBuffer() override;

	virtual void Bind()   const override;
	virtual void Unbind() const override;

	virtual void SetLayout(const BufferLayout& layout) override { this->layout = layout; }
	virtual const BufferLayout& GetLayout()	const	   override	{ return layout;		 }

private:
	unsigned int id;
	BufferLayout layout;
};

}