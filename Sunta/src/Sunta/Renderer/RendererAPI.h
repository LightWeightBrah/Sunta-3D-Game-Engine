#pragma once


namespace Sunta
{

struct RendererConfig
{
	std::string GLSLVersion;
	std::string ImGuiGLSLVersion;
	int OpenGLMajor;
	int OpenGLMinor;
};

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

	static const RendererConfig& GetConfig() { return rendererConfig; }
private:
	inline static API usedAPI;
	inline static RendererConfig rendererConfig;

	static bool IsSupported(API targetedAPI);
	static const char* GetAPIName(API targetedAPI);
	static void ConfigureRendererSpecs();
};

}