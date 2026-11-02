#include "Core/SuntaPreCompiled.h"
#include "PostProcessing.h"

#include <glm/glm.hpp>

#include "ElementBuffer.h"
#include "Framebuffer.h"
#include "Renderer.h"
#include "RendererDevice.h"
#include "ScopedVertexArrayBind.h"
#include "Shader.h"
#include "VertexArray.h"
#include "VertexBuffer.h"
#include "VertexLayouts.h"
#include "Core/VirtualFileSystem.h"

namespace Sunta
{

namespace
{

constexpr const char* BrightPassShaderPath = "@engine/Shaders/BloomBrightPass.shader";
constexpr const char* BlurShaderPath       = "@engine/Shaders/BloomBlur.shader";
constexpr const char* CompositeShaderPath  = "@engine/Shaders/Composite.shader";

// The glow is made in a picture this many times smaller than the screen: faster, and the blur reaches farther (a quarter: a glow wide enough to see around a fire)
constexpr int BloomDownscale = 4;

constexpr int MaxBlurPasses = 20;

// Texture slots of the composite shader
constexpr unsigned int SceneSlot = 0;
constexpr unsigned int BloomSlot = 1;

// The corners of the screen in clip space (-1..1); the shaders make the texture coordinates from them
const glm::vec3 QuadCorners[] =
{
	{ -1.0f, -1.0f, 0.0f }, { 1.0f, -1.0f, 0.0f }, { 1.0f, 1.0f, 0.0f }, { -1.0f, 1.0f, 0.0f }
};

const unsigned int QuadIndices[] = { 0, 1, 2, 2, 3, 0 };

std::shared_ptr<Framebuffer> CreateTarget(int width, int height, bool hasDepth)
{
	FramebufferSpecification specification;
	specification.width       = width;
	specification.height      = height;
	specification.attachments = { FramebufferAttachment::RGBA16F };

	// The scene needs its depth (walls hide what is behind them), the glow doesn't
	if (hasDepth)
		specification.attachments.push_back(FramebufferAttachment::Depth24);

	return Renderer::GetDevice().CreateFramebuffer(specification);
}

void ResizeOrCreate(std::shared_ptr<Framebuffer>& target, int width, int height, bool hasDepth)
{
	if (!target)
		target = CreateTarget(width, height, hasDepth);
	else
		target->Resize(width, height);
}

}

void PostProcessing::EnsureInitialized()
{
	if (screenQuad)
		return;

	RendererDevice& rendererDevice = Renderer::GetDevice();

	screenQuad = rendererDevice.CreateVertexArray();
	ScopedVertexArrayBind vertexArrayGuard(*screenQuad);

	BufferDescriptor vertexBufferDescriptor;
	vertexBufferDescriptor.data = QuadCorners;
	vertexBufferDescriptor.size = sizeof(QuadCorners);

	auto vertexBuffer = rendererDevice.CreateVertexBuffer(vertexBufferDescriptor);
	vertexBuffer->SetLayout(VertexLayouts::GetPositionOnlyLayout());

	BufferDescriptor elementBufferDescriptor;
	elementBufferDescriptor.data = QuadIndices;
	elementBufferDescriptor.size = sizeof(QuadIndices);

	screenQuad->AddVertexBuffer(vertexBuffer);
	screenQuad->SetElementBuffer(rendererDevice.CreateElementBuffer(elementBufferDescriptor));

	brightPassShader = rendererDevice.CreateShader(VirtualFileSystem::Resolve(BrightPassShaderPath));
	blurShader       = rendererDevice.CreateShader(VirtualFileSystem::Resolve(BlurShaderPath));
	compositeShader  = rendererDevice.CreateShader(VirtualFileSystem::Resolve(CompositeShaderPath));
}

void PostProcessing::EnsureTargets(int width, int height)
{
	ResizeOrCreate(sceneTarget, width, height, true);

	const int bloomWidth  = glm::max(width  / BloomDownscale, 1);
	const int bloomHeight = glm::max(height / BloomDownscale, 1);

	for (auto& bloomTarget : bloomTargets)
		ResizeOrCreate(bloomTarget, bloomWidth, bloomHeight, false);
}

void PostProcessing::DrawScreenQuad()
{
	screenQuad->Bind();
	Renderer::GetDevice().DrawElements(*screenQuad);
}

bool PostProcessing::Begin(int width, int height, float clearR, float clearG, float clearB)
{
	if (width <= 0 || height <= 0)
		return false;

	EnsureInitialized();
	EnsureTargets(width, height);

	if (!sceneTarget->IsComplete() || !bloomTargets[0]->IsComplete() || !bloomTargets[1]->IsComplete())
		return false;

	// Ended in End: the scene is drawn into it until then
	sceneTarget->Bind();
	sceneTarget->Clear(clearR, clearG, clearB, 1.0f);
	return true;
}

int PostProcessing::MakeBloom(const PostProcessSettings& settings)
{
	// 1. Only what is brighter than the threshold
	{
		ScopedFramebufferBind bind(*bloomTargets[0]);
		bloomTargets[0]->Clear();

		brightPassShader->Bind();
		sceneTarget->BindColorTexture(0, SceneSlot);
		brightPassShader->SetUniform1i("scene", SceneSlot);
		brightPassShader->SetUniform1f("threshold", settings.bloomThreshold);
		DrawScreenQuad();
	}

	// 2. Blurred horizontally into the other target, then vertically back, and again (a Gaussian blur in two steps
	// costs 2 x 9 samples per pixel instead of 9 x 9)
	blurShader->Bind();
	blurShader->SetUniform1i("image", SceneSlot);

	int source = 0;
	const int passes = glm::clamp(settings.bloomBlurPasses, 1, MaxBlurPasses) * 2;

	for (int i = 0; i < passes; ++i)
	{
		const int target = 1 - source;

		ScopedFramebufferBind bind(*bloomTargets[target]);
		bloomTargets[source]->BindColorTexture(0, SceneSlot);
		blurShader->SetUniformBool("isHorizontal", i % 2 == 0);
		DrawScreenQuad();

		source = target;
	}

	return source;
}

void PostProcessing::End(const PostProcessSettings& settings)
{
	sceneTarget->Unbind();

	RendererDevice& rendererDevice = Renderer::GetDevice();

	// The passes draw flat pictures over everything: no depth, no blending
	rendererDevice.SetDepthTest(false);
	rendererDevice.SetAlphaBlending(false);

	const int bloom = settings.isBloomEnabled ? MakeBloom(settings) : -1;

	// 3. On the screen: the scene with the glow, tone mapped
	compositeShader->Bind();
	sceneTarget->BindColorTexture(0, SceneSlot);
	compositeShader->SetUniform1i("scene", SceneSlot);

	compositeShader->SetUniformBool("hasBloom", bloom >= 0);
	if (bloom >= 0)
	{
		bloomTargets[bloom]->BindColorTexture(0, BloomSlot);
		compositeShader->SetUniform1i("bloom", BloomSlot);
		compositeShader->SetUniform1f("bloomIntensity", settings.bloomIntensity);
	}

	compositeShader->SetUniform1f("exposure", settings.exposure);
	DrawScreenQuad();

	rendererDevice.SetDepthTest(true);
}

}
