#pragma once

namespace Sunta
{

class RendererAPI
{
public:
	enum class API
	{
		None = 0,
		OpenGL,
		Vulkan,
		DirectX12,
		Metal
	};

	static API GetAPI() { return usedAPI; }
private:
	inline static API usedAPI;
};

}