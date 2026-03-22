#include "GraphicsContext.h"
#include "Core/Assert.h"
#include "Platform/OpenGL/OpenGLGraphicsContext.h"
#include "RendererAPI.h"


namespace Sunta
{

void GraphicsContext::Configure()
{
	switch (RendererAPI::GetAPI())
	{
	case RendererAPI::API::OpenGL:    OpenGLGraphicsContext::Configure(); return;
	case RendererAPI::API::Vulkan:    break; // TODO: ADD VULKAN    API
	case RendererAPI::API::DirectX12: break; // TODO: ADD DIRECTX12 API
	case RendererAPI::API::Metal:     break; // TODO: ADD METAL     API
	}

	SUNTA_ASSERT(false, "Unknown Renderer API in Graphics Context Configuration!!!");
}

std::unique_ptr<GraphicsContext> GraphicsContext::Create(void* window)
{
	switch (RendererAPI::GetAPI())
	{
	case RendererAPI::API::OpenGL:    return std::make_unique<OpenGLGraphicsContext>((GLFWwindow*)window);
	case RendererAPI::API::Vulkan:    break; // TODO: ADD VULKAN    API
	case RendererAPI::API::DirectX12: break; // TODO: ADD DIRECTX12 API
	case RendererAPI::API::Metal:     break; // TODO: ADD METAL     API
	}

	SUNTA_ASSERT(false, "Unknown Renderer API!!!");

}

}