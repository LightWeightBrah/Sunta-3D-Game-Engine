#pragma once

namespace Sunta
{
	class VertexBuffer
	{
	private:
		unsigned int id;
	public:
		VertexBuffer(const void* data, unsigned int size);
		~VertexBuffer();
	
		void Bind() const;
		void Unbind() const;
	};
}