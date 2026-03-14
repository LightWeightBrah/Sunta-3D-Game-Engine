#pragma once

namespace Sunta
{
	class OpenGLBufferLayout;
	class OpenGLVertexBuffer;
	
	class OpenGLVertexArray
	{
	private:
		unsigned int id;
	public:
		OpenGLVertexArray();
		~OpenGLVertexArray();
	
		void AddBuffer(const OpenGLVertexBuffer& VBO, const OpenGLBufferLayout& layout);
	
		void Bind() const;
		void Unbind() const;
	};
}