#include "Core/SuntaPreCompiled.h"
#include "RendererAPI.h"

#include "Core/Log.h"

namespace Sunta
{

void RendererAPI::SetAPI(API targetedAPI)
{
	if (IsSupported(targetedAPI))
	{
		SUNTA_ENGINE_LOG_INFO("Selected Renderer API ({0})", GetAPIName(targetedAPI));
		usedAPI = targetedAPI;
	}
	else
	{
		usedAPI = API::OpenGL;
		SUNTA_ENGINE_LOG_WARNING("Selected Renderer API ({0}) is not supported! Using default OpenGL API", GetAPIName(targetedAPI));
	}
}

bool RendererAPI::IsSupported(API targetedAPI)
{
	switch (targetedAPI)
	{
	case API::OpenGL:      return true;
	case API::Vulkan:      return false;
	case API::DirectX12:   return false;
	case API::Metal:       return false;
	default:               return false;
	}
}

const char* RendererAPI::GetAPIName(API targetedAPI)
{
	switch (targetedAPI)
	{
	case API::OpenGL:      return "OpenGL";
	case API::Vulkan:      return "Vulkan";
	case API::DirectX12:   return "DirectX12";
	case API::Metal:       return "Metal";
	default:               return "None";
	}
}

}