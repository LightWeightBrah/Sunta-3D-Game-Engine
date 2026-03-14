#pragma once

namespace Sunta
{
	class VertexBuffer
	{
	public:
		// We need a virtual destructor = default so the 
		// derived classes uses their own destructors B : A 
		// (using desctutor base classs A and destructor of derived class B
		virtual ~VertexBuffer() = default;
	
		virtual void Bind()    const = 0;
		virtual void Unbind()  const = 0;
	};
}