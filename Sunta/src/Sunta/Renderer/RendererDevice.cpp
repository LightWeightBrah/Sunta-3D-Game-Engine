#include "RendererDevice.h"

#include "RendererAPI.h"
#include "Platform/OpenGL/OpenGLDevice.h"
#include "Core/Assert.h"

namespace Sunta
{

std::unique_ptr<Sunta::RendererDevice> RendererDevice::Create()
{
	switch (RendererAPI::GetAPI())
	{
	case RendererAPI::API::OpenGL:    return std::make_unique<OpenGLDevice>();
	case RendererAPI::API::Vulkan:    break; // TODO: ADD VULKAN    API
	case RendererAPI::API::DirectX12: break; // TODO: ADD DIRECTX12 API
	case RendererAPI::API::Metal:     break; // TODO: ADD METAL     API

	}

	SUNTA_ASSERT(false, "Unknown Renderer API!!!");
	return nullptr;
}

}