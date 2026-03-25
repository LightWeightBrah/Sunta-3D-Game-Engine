#pragma once

namespace Sunta
{

class ElementBuffer
{
public:
	virtual ~ElementBuffer() = default;

	virtual void Bind() const = 0;
	virtual void Unbind() const = 0;

	virtual unsigned int GetCount() const = 0;
};

}