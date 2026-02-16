#pragma once
#include <vector>
#include <GL/glew.h>

struct BufferElement
{
	unsigned int		type;
	unsigned int		count;
	bool				normalized;

	static inline unsigned int GetSizeOfType(unsigned int type)
	{
		switch (type)
		{
		case GL_FLOAT:				return 4;
		case GL_UNSIGNED_INT:		return 4;
		case GL_UNSIGNED_BYTE:		return 1;
		}

		return 0;
	}
};

class BufferLayout
{
public:
	template<typename T>
	void Push(unsigned int count, bool normalized = false)
	{
		static_assert(sizeof(T) == 0, "Error: Added unsupported type of Buffer Layout");
	}
	
	template<>
	void Push<float>(unsigned int count, bool normalized)
	{
		bufferElements.push_back({ GL_FLOAT, count, normalized});
		stride += count * BufferElement::GetSizeOfType(GL_FLOAT);
	}

	template<>
	void Push<unsigned int>(unsigned int count, bool normalized)
	{
		bufferElements.push_back({ GL_UNSIGNED_INT, count, normalized });
		stride += count * BufferElement::GetSizeOfType(GL_UNSIGNED_INT);
	}

	inline const std::vector<BufferElement>& GetBufferElements() const { return bufferElements; }
	inline unsigned int GetStride() const { return stride; }

private:
	std::vector<BufferElement>		bufferElements;
	unsigned int					stride = 0;
};