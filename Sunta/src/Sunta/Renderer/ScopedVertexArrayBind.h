#pragma once

#include "VertexArray.h"

namespace Sunta
{

class ScopedVertexArrayBind
{

public:
	// prohibit implicit conversion, so when creating object it HAS TO BE of type ScopedVertexArrayBind and not VertexArray
	explicit ScopedVertexArrayBind(VertexArray& vertexArray)
		: vertexArray(vertexArray)
	{
		vertexArray.Bind();
	}

	~ScopedVertexArrayBind()
	{
		vertexArray.Unbind();
	}

	ScopedVertexArrayBind(const ScopedVertexArrayBind&)            = delete;  // no copy constructor
	ScopedVertexArrayBind& operator=(const ScopedVertexArrayBind&) = delete;  // no assign operator


private:
	VertexArray& vertexArray;
};

}