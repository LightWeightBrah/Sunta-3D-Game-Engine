#pragma once

class BufferLayout;
class VertexBuffer;

class VertexArray
{
private:
	unsigned int id;
public:
	VertexArray();
	~VertexArray();

	void AddBuffer(const VertexBuffer& VBO, const BufferLayout& layout);

	void Bind() const;
	void Unbind() const;
};