#include "Core/SuntaPreCompiled.h"

#include <glad/glad.h>

#include "OpenGLVertexArray.h"

#include "Renderer/VertexBuffer.h"
#include "Renderer/ElementBuffer.h"
#include "Renderer/BufferLayout.h"
#include "OpenGLUtilities.h"

namespace Sunta
{
	OpenGLVertexArray::OpenGLVertexArray()
	{
		GLCall(glGenVertexArrays(1, &id));
	}
	
	OpenGLVertexArray::~OpenGLVertexArray()
	{
		GLCall(glDeleteVertexArrays(1, &id));
	}

	void OpenGLVertexArray::Bind() const
	{
		GLCall(glBindVertexArray(id));
	}

	void OpenGLVertexArray::Unbind() const
	{
		GLCall(glBindVertexArray(0));
	}
	
	void OpenGLVertexArray::AddVertexBuffer(const std::shared_ptr<VertexBuffer>& vertexBuffer)
	{
		SUNTA_ASSERT(vertexBuffer.GetLayout().GetBufferLayoutElements().size(), "VERTEX BUFFER HAS NO LAYOUT!!!");

		GLCall(glBindVertexArray(id));
		vertexBuffer->Bind();

		const auto& bufferLayout = vertexBuffer->GetLayout();
		const auto& elements = bufferLayout.GetBufferLayoutElements();

		for (unsigned int i = 0; i < elements.size(); i++)
		{
			const auto& element = elements[i];

			switch (element.shaderDataType)
			{
			case ShaderDataType::Float:
			case ShaderDataType::Float2:
			case ShaderDataType::Float3:
			case ShaderDataType::Float4:
			{
				GLCall(glEnableVertexAttribArray(attributeIndex));
				GLCall(glVertexAttribPointer(attributeIndex
					, element.GetComponentCount()
					, ShaderDataTypeToGLenum(element.shaderDataType)
					, element.normalized ? GL_TRUE : GL_FALSE
					, bufferLayout.GetStride()
					, (const void*)(element.offset)));

				ActivateInstancing(attributeIndex, element.instanced);
				attributeIndex++;

				break;
			}

			case ShaderDataType::Mat3:
			case ShaderDataType::Mat4:
			{
				// Matricies vertexAttribPointer (mainly for instancing)

				// count is how much elements are in Matrix Column 
				// e.g for 3x3 Matrix we have count = 3
				// e.g for 4x4 Matrix we have count = 4
				unsigned int count = element.GetComponentCount();
				for (unsigned int i = 0; i < count; i++)
				{
					GLCall(glEnableVertexAttribArray(attributeIndex));
					GLCall(glVertexAttribPointer(attributeIndex
						, count
						, ShaderDataTypeToGLenum(element.shaderDataType)
						, element.normalized ? GL_TRUE : GL_FALSE
						, bufferLayout.GetStride()
						, (const void*)(element.offset + sizeof(float) * count * i)));
					
					ActivateInstancing(attributeIndex, element.instanced);
					attributeIndex++;
				}

				break;
			}

			case ShaderDataType::Int:
			case ShaderDataType::Int2:
			case ShaderDataType::Int3:
			case ShaderDataType::Int4:
			case ShaderDataType::Bool:
			{
				GLCall(glEnableVertexAttribArray(attributeIndex));
				GLCall(glVertexAttribIPointer(attributeIndex
					, element.GetComponentCount()
					, ShaderDataTypeToGLenum(element.shaderDataType)
					, bufferLayout.GetStride()
					, (const void*)element.offset));

				ActivateInstancing(attributeIndex, element.instanced);
				attributeIndex++;

				break;
			}

			default:
				SUNTA_ASSERT(false, "UNKNOWN SHADER DATA TYPE!!!");

			}

			vertexBuffers.push_back(vertexBuffer);
		}
	}

	void OpenGLVertexArray::SetElementBuffer(const std::shared_ptr<ElementBuffer>& elementBuffer)
	{
		GLCall(glBindVertexArray(id));
		elementBuffer->Bind();
		this->elementBuffer = elementBuffer;
	}

	void OpenGLVertexArray::ActivateInstancing(unsigned int attributeIndex, bool instanced)
	{
		// glVertexAttribDivisor allows us to draw same layout (location=attributeIndex)
		// per draw instance instead of per vertex so we can have a lot of instances and 1 draw call
		if (instanced)
			GLCall(glVertexAttribDivisor(attributeIndex, 1));
		else
			GLCall(glVertexAttribDivisor(attributeIndex, 0));

	}

}
