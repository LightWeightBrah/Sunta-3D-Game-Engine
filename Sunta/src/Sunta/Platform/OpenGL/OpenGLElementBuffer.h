#pragma once

#include "Renderer/ElementBuffer.h"

namespace Sunta
{
	class OpenGLElementBuffer : public ElementBuffer
	{
	private:
		unsigned int id;
		unsigned int count;
	public:
		OpenGLElementBuffer(const unsigned int* data, unsigned int size);
		virtual ~OpenGLElementBuffer() override;
	
		virtual void Bind() const override;
		virtual void Unbind() const override;

		virtual unsigned int GetCount() const override { return count; }
	
	};
}