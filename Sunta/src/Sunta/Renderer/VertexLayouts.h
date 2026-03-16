#pragma once
#include "BufferLayout.h"
#include "Core/Config.h"

namespace Sunta
{
	namespace VertexLayouts
	{
		static BufferLayout GetStaticLayout()
		{
			BufferLayout bufferLayout
			{
				{ ShaderDataType::Float3, "aPos"      },
				{ ShaderDataType::Float3, "aNormal"   },
				{ ShaderDataType::Float3, "aTexCoord" }
			};

			//bufferLayout.Push<float>(3); //position
			//bufferLayout.Push<float>(3); //normal
			//bufferLayout.Push<float>(2); //tex coords
	
			return bufferLayout;
		}
	
		static BufferLayout GetSkinnedLayout()
		{
			BufferLayout bufferLayout
			{
				{ ShaderDataType::Float3, "aPos"      },
				{ ShaderDataType::Float3, "aNormal"   },
				{ ShaderDataType::Float3, "aTexCoord" },

				{ ShaderDataType::Float3, "aBoneIDs"  },
				{ ShaderDataType::Float3, "aWeights"  }
			};

			//BufferLayout bufferLayout = GetStaticLayout();
			//bufferLayout.Push<unsigned int>(MAX_NUM_BONES_PER_VERTEX); //bone ids
			//bufferLayout.Push<float>	   (MAX_NUM_BONES_PER_VERTEX); //weights
	
			return bufferLayout;
		}
	}
}