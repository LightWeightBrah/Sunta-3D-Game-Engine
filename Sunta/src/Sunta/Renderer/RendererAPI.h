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
	static void SetAPI(API targetedAPI);
private:
	inline static API usedAPI;

	static bool IsSupported(API targetedAPI);
	static const char* GetAPIName(API targetedAPI);
};

}