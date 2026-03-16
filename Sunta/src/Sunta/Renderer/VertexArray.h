	#pragma once

	namespace Sunta
	{
		class BufferLayout;
		class VertexBuffer;
	
		class VertexArray
		{
		public:
			~VertexArray() = default;
	
			virtual void Bind() const = 0;
			virtual void Unbind() const = 0;

			virtual void AddVertexBuffer(const std::shared_ptr<VertexBuffer>& VBO) = 0;
			virtual void SetElementBuffer(const std::shared_ptr<ElementBuffer>& EBO) = 0;

			virtual const std::vector<std::shared_ptr<VertexBuffer>>& GetVertexBuffer() const = 0;
			virtual const std::shared_ptr<ElementBuffer>& GetElementBuffer() const = 0;
		};
	}