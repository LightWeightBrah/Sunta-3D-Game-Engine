#pragma once
#include <vector>
#include <glad/glad.h>

namespace Sunta
{

enum class ShaderDataType
{
	None = 0,
	Float,
	Float2,
	Float3,
	Float4,

	Mat3,
	Mat4,

	Int,
	Int2,
	Int3,
	Int4,

	Bool
};

struct BufferLayoutElement
{
	ShaderDataType	shaderDataType;
	std::string		name;
	unsigned int	offset;
	unsigned int	size;

	BufferLayoutElement(ShaderDataType shaderDataType, const std::string& name)
		: shaderDataType(shaderDataType)
		, name(name)
		, offset(0)
		, size(CalculateSize(shaderDataType))
	{

	}

	static unsigned int CalculateSize(ShaderDataType shaderDataType)
	{
		switch (shaderDataType)
		{
		case ShaderDataType::Float:  return 4;
		case ShaderDataType::Float2: return 4 * 2;
		case ShaderDataType::Float3: return 4 * 3;
		case ShaderDataType::Float4: return 4 * 4;

		case ShaderDataType::Mat3:   return 4 * 3 * 3;
		case ShaderDataType::Mat4:   return 4 * 4 * 4;
									 
		case ShaderDataType::Int:    return 4;
		case ShaderDataType::Int2:   return 4 * 2;
		case ShaderDataType::Int3:   return 4 * 3;
		case ShaderDataType::Int4:   return 4 * 4;
								     
		case ShaderDataType::Bool:   return 1;
		}

		return 0;
	}
		
};
	
class BufferLayout
{
public:
	BufferLayout(const std::intializer_list<BufferLayoutElement>& bufferElements)
		: bufferElements(bufferElements)
	{
		unsigned int offset = 0;
		for (auto& element : bufferElements)
		{
			element.offset = offset;
			offset += element.size;
			stride += element.size;
		}
	}


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

}