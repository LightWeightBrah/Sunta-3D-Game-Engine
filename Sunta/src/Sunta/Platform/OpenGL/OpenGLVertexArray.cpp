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

			GLCall(glEnableVertexAttribArray(i));

			switch (element.shaderDataType)
			{
			case ShaderDataType::Float:
			case ShaderDataType::Float2:
			case ShaderDataType::Float3:
			case ShaderDataType::Float4:
			{
				GLCall(glVertexAttribPointer(vertexBufferIndex
					, element.GetComponentCount()
					, ShaderDataTypeToOpenGLBaseType(element.shaderDataType)
					, element.normalized ? GL_TRUE : GL_FALSE
					, bufferLayout.GetStride()
					, (const void*)(element.offset)));

				break;
			}

			case ShaderDataType::Mat3:
			case ShaderDataType::Mat4:
			{
				//TODO: add Matricies vertexAttribPointer
				break;
			}

			case ShaderDataType::Int:
			case ShaderDataType::Int2:
			case ShaderDataType::Int3:
			case ShaderDataType::Int4:
			case ShaderDataType::Bool:
			{
				GLCall(glVertexAttribIPointer(vertexBufferIndex
					, element.GetComponentCount()
					, ShaderDataTypeToOpenGLBaseType(element.shaderDataType)
					, bufferLayout.GetStride()
					, (const void*)element.offset));

				break;
			}

			default:
				SUNTA_ASSERT(false, "UNKNOWN SHADER DATA TYPE!!!");

			}

			vertexBufferIndex++;
			vertexBuffers.push_back(vertexBuffer);
		}
	}

	void OpenGLVertexArray::SetElementBuffer(const std::shared_ptr<ElementBuffer>& elementBuffer)
	{

	}

}
