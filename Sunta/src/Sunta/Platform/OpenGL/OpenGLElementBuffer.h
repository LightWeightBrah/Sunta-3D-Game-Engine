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
		~OpenGLElementBuffer();
	
		void Bind() const;
		void Unbind() const;
		inline unsigned int GetCount() const { return count; }
	
	};
}