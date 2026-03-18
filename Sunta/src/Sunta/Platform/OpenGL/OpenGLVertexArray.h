#pragma once

#include "Renderer/VertexArray.h"

namespace Sunta
{

class VertexBuffer;
class ElementBuffer;

class OpenGLVertexArray : public VertexArray
{
public:
	OpenGLVertexArray();
	~OpenGLVertexArray();

	void Bind()		const;
	void Unbind()	const;

	virtual void AddVertexBuffer(const std::shared_ptr<VertexBuffer>& vertexBuffer)	   override;
	virtual void SetElementBuffer(const std::shared_ptr<ElementBuffer>& elementBuffer) override;

	virtual const std::vector<std::shared_ptr<VertexBuffer>>& GetVertexBuffer()  const override { return vertexBuffers; }
	virtual const std::shared_ptr<ElementBuffer>& GetElementBuffer()			 const override { return elementBuffer; } 

private:
	unsigned int id;
	unsigned int vertexBufferIndex;

	std::vector<std::shared_ptr<VertexBuffer>> vertexBuffers;
	std::shared_ptr<ElementBuffer>			   elementBuffer;
};

}