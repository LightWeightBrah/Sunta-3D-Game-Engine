#pragma once

#include "Renderer/ElementBuffer.h"

namespace Sunta
{

struct BufferDescriptor;

class OpenGLElementBuffer : public ElementBuffer
{
public:
	OpenGLElementBuffer(const BufferDescriptor& descriptor);
	virtual ~OpenGLElementBuffer() override;

	virtual void Bind() const override;
	virtual void Unbind() const override;

	virtual unsigned int GetCount() const override { return count; }

private:
	unsigned int id;
	unsigned int count;
};

}