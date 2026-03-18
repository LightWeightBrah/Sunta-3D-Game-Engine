#pragma once
#include <vector>
#include <Core/Assert.h>

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

static unsigned int CalculateDataTypeSize(ShaderDataType shaderDataType)
{
	switch (shaderDataType)
	{
	case ShaderDataType::Float:  return 4;
	case ShaderDataType::Float2: return 4 * 2;
	case ShaderDataType::Float3: return 4 * 3;
	case ShaderDataType::Float4: return 4 * 4;

	case ShaderDataType::Mat3:   return 4 * 3 * 3; // 3 * Float3
	case ShaderDataType::Mat4:   return 4 * 4 * 4; // 4 * Float4

	case ShaderDataType::Int:    return 4;
	case ShaderDataType::Int2:   return 4 * 2;
	case ShaderDataType::Int3:   return 4 * 3;
	case ShaderDataType::Int4:   return 4 * 4;

	case ShaderDataType::Bool:   return 4; // bool actually is send as 4 bytes instead of 1 byte to match memory alignment in GPU
	}

	SUNTA_ASSERT(false, "UNKNOWN SHADER DATA TYPE!!!");
	return 0;
}

struct BufferLayoutElement
{
	ShaderDataType	shaderDataType;
	std::string		name;
	bool			normalized;
	bool			instanced;

	unsigned int	size;
	unsigned int	offset;

	BufferLayoutElement(ShaderDataType shaderDataType, const std::string& name, bool normalized = false, bool instanced = false)
		: shaderDataType(shaderDataType)
		, name(name)
		, normalized(normalized)
		, instanced(instanced)
		, size(CalculateDataTypeSize(shaderDataType))
		, offset(0)
	{

	}

	unsigned int GetComponentCount() const
	{
		switch (shaderDataType)
		{
		case ShaderDataType::Float:  return 1;
		case ShaderDataType::Float2: return 2;
		case ShaderDataType::Float3: return 3;
		case ShaderDataType::Float4: return 4;

		case ShaderDataType::Mat3:   return 3; // layout (location = 0) Takes 3 * Float3, so next location is (location = 3)
		case ShaderDataType::Mat4:   return 4; // layout (location = 0) Takes 4 * Float4, so next location is (location = 4)

		case ShaderDataType::Int:    return 1;
		case ShaderDataType::Int2:   return 2;
		case ShaderDataType::Int3:   return 3;
		case ShaderDataType::Int4:   return 4;

		case ShaderDataType::Bool:   return 1;
		}

		SUNTA_ASSERT(false, "UNKNOWN SHADER DATA TYPE!!!");
		return 0;
	}
};
	
class BufferLayout
{
public:
	BufferLayout(const std::initializer_list<BufferLayoutElement>& bufferLayoutElements)
		: bufferLayoutElements(bufferLayoutElements)
	{
		CalculateOffsetsAndStride();
	}

	inline const std::vector<BufferLayoutElement>& GetBufferLayoutElements() const { return bufferLayoutElements; }
	inline unsigned int GetStride() const { return stride; }

private:
	void CalculateOffsetsAndStride()
	{
		unsigned int offset = 0;
		this->stride = 0;

		for (auto& layoutElement : bufferLayoutElements)
		{
			layoutElement.offset = offset;
			offset += layoutElement.size;
			stride += layoutElement.size;
		}
	}

	std::vector<BufferLayoutElement> bufferLayoutElements;
	unsigned int					 stride = 0;
};

}