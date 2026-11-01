#include "Core/SuntaPreCompiled.h"
#include "SkyboxRenderer.h"

#include "Cubemap.h"
#include "ElementBuffer.h"
#include "Renderer.h"
#include "RendererDevice.h"
#include "ScopedVertexArrayBind.h"
#include "Shader.h"
#include "VertexArray.h"
#include "VertexBuffer.h"
#include "VertexLayouts.h"
#include "Core/Log.h"
#include "Core/VirtualFileSystem.h"

namespace Sunta
{

namespace
{

constexpr const char* ShaderPath = "@engine/Shaders/Skybox.shader";

// A cube from -1 to 1: its corners are also the directions the sky is read in
const glm::vec3 CubeCorners[] =
{
	{ -1.0f, -1.0f, -1.0f }, {  1.0f, -1.0f, -1.0f }, {  1.0f,  1.0f, -1.0f }, { -1.0f,  1.0f, -1.0f },
	{ -1.0f, -1.0f,  1.0f }, {  1.0f, -1.0f,  1.0f }, {  1.0f,  1.0f,  1.0f }, { -1.0f,  1.0f,  1.0f }
};

const unsigned int CubeIndices[] =
{
	0, 1, 2, 2, 3, 0, // back
	4, 6, 5, 6, 4, 7, // front
	0, 3, 7, 7, 4, 0, // left
	1, 5, 6, 6, 2, 1, // right
	3, 2, 6, 6, 7, 3, // top
	0, 4, 5, 5, 1, 0  // bottom
};

}

void SkyboxRenderer::EnsureInitialized()
{
	if (shader)
		return;

	RendererDevice& rendererDevice = Renderer::GetDevice();

	cube = rendererDevice.CreateVertexArray();
	ScopedVertexArrayBind vertexArrayGuard(*cube);

	BufferDescriptor vertexBufferDescriptor;
	vertexBufferDescriptor.data = CubeCorners;
	vertexBufferDescriptor.size = sizeof(CubeCorners);

	auto vertexBuffer = rendererDevice.CreateVertexBuffer(vertexBufferDescriptor);
	vertexBuffer->SetLayout(VertexLayouts::GetPositionOnlyLayout());

	BufferDescriptor elementBufferDescriptor;
	elementBufferDescriptor.data = CubeIndices;
	elementBufferDescriptor.size = sizeof(CubeIndices);

	cube->AddVertexBuffer(vertexBuffer);
	cube->SetElementBuffer(rendererDevice.CreateElementBuffer(elementBufferDescriptor));

	shader = rendererDevice.CreateShader(VirtualFileSystem::Resolve(ShaderPath));
}

Cubemap* SkyboxRenderer::GetCubemap(const std::string& folder)
{
	auto found = cubemaps.find(folder);
	if (found == cubemaps.end())
	{
		// Loaded once, also when it failed (the error is logged once, not every frame)
		found = cubemaps.emplace(folder, Renderer::GetDevice().CreateCubemap(VirtualFileSystem::Resolve(folder))).first;
	}

	return (found->second && found->second->IsLoaded()) ? found->second.get() : nullptr;
}

bool SkyboxRenderer::Draw(const std::string& folder, const glm::mat4& view, const glm::mat4& projection, float brightness)
{
	if (folder.empty())
		return false;

	EnsureInitialized();

	Cubemap* sky = GetCubemap(folder);
	if (!sky)
		return false;

	RendererDevice& rendererDevice = Renderer::GetDevice();
	rendererDevice.SetDepthWrite(false);

	shader->Bind();
	shader->SetUniformMatrix4fv("viewRotation", glm::mat4(glm::mat3(view)));
	shader->SetUniformMatrix4fv("projection", projection);
	shader->SetUniform1f("brightness", brightness);

	sky->Bind(0);
	shader->SetUniform1i("sky", 0);

	rendererDevice.DrawElements(*cube);

	rendererDevice.SetDepthWrite(true);
	return true;
}

}
