#pragma once
#include "BufferLayout.h"
#include "Config.h"

namespace VertexLayouts
{
	static BufferLayout GetStaticLayout()
	{
		BufferLayout bufferLayout;
		bufferLayout.Push<float>(3); //position
		bufferLayout.Push<float>(3); //normal
		bufferLayout.Push<float>(2); //tex coords

		return bufferLayout;
	}

	static BufferLayout GetSkinnedLayout()
	{
		BufferLayout bufferLayout = GetStaticLayout();
		
		bufferLayout.Push<unsigned int>(MAX_NUM_BONES_PER_VERTEX); //bone ids
		bufferLayout.Push<float>	   (MAX_NUM_BONES_PER_VERTEX); //weights

		return bufferLayout;
	}
}