#pragma once

namespace Sunta
{

class BufferLayout;

class VertexBuffer
{
public:
	// We need a virtual destructor = default so the 
	// derived classes uses their own destructors B : A 
	// (using desctutor base classs A and destructor of derived class B
	virtual ~VertexBuffer() = default;
	
	virtual void Bind()    const = 0;
	virtual void Unbind()  const = 0;

	virtual void SetLayout(const BufferLayout& layout) = 0;
	virtual const BufferLayout& GetLayout()	const = 0;

	virtual void UpdateDynamicData(const void* data, unsigned int size) = 0;
};

}