#include "Core/SuntaPreCompiled.h"
#include "PostProcessing.h"

#include <chrono>
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
constexpr const char* PixelArtShaderPath   = "@engine/Shaders/PixelArt.shader";

// The glow is made in a picture this many times smaller than the screen: faster, and the blur reaches farther (a quarter: a glow wide enough to see around a fire)
constexpr int BloomDownscale = 4;

constexpr int MaxBlurPasses = 20;

// Texture slots of the composite shader
constexpr unsigned int SceneSlot = 0;
constexpr unsigned int BloomSlot = 1;
constexpr unsigned int DepthSlot = 2; // the scene's depth, for the outlines of pixel art

// The corners of the screen in clip space (-1..1); the shaders make the texture coordinates from them
const glm::vec3 QuadCorners[] =
{
	{ -1.0f, -1.0f, 0.0f }, { 1.0f, -1.0f, 0.0f }, { 1.0f, 1.0f, 0.0f }, { -1.0f, 1.0f, 0.0f }
};

const unsigned int QuadIndices[] = { 0, 1, 2, 2, 3, 0 };

// The impact frame being played (see PostProcessing::Kick), timed by the wall clock: hit-stops freeze the game, not it
struct ImpactFrame
{
	float strength = 0.0f;
	float seconds  = 0.3f;
	std::chrono::steady_clock::time_point start;
};

ImpactFrame& GetImpact()
{
	static ImpactFrame impact;
	return impact;
}

float CurrentImpact()
{
	const ImpactFrame& impact = GetImpact();
	if (impact.strength <= 0.0f)
		return 0.0f;

	const float age = std::chrono::duration<float>(std::chrono::steady_clock::now() - impact.start).count();
	const float left = 1.0f - glm::clamp(age / glm::max(impact.seconds, 0.01f), 0.0f, 1.0f);
	return impact.strength * left * left;
}

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
	pixelArtShader   = rendererDevice.CreateShader(VirtualFileSystem::Resolve(PixelArtShaderPath));
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
	isDrawingScene = true;
	sceneTarget->Clear(clearR, clearG, clearB, 1.0f);
	return true;
}

bool PostProcessing::CopySceneForSeeThrough()
{
	if (!isDrawingScene || !sceneTarget)
		return false;

	const FramebufferSpecification& scene = sceneTarget->GetSpecification();
	ResizeOrCreate(sceneCopy, scene.width, scene.height, false);
	if (!sceneCopy->IsComplete())
		return false;

	sceneTarget->BlitColorTo(*sceneCopy);
	hasSceneCopy = true;
	return true;
}

void PostProcessing::BindSceneCopy(unsigned int slot)
{
	if (sceneCopy)
		sceneCopy->BindColorTexture(0, slot);
}

glm::vec2 PostProcessing::GetSceneCopySize()
{
	if (!sceneCopy)
		return glm::vec2(1.0f);
	const FramebufferSpecification& copy = sceneCopy->GetSpecification();
	return glm::vec2((float)copy.width, (float)copy.height);
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
	isDrawingScene = false;
	hasSceneCopy   = false;

	RendererDevice& rendererDevice = Renderer::GetDevice();

	// The passes draw flat pictures over everything: no depth, no blending
	rendererDevice.SetDepthTest(false);
	rendererDevice.SetAlphaBlending(false);

	const int bloom = settings.isBloomEnabled ? MakeBloom(settings) : -1;

	// Pixel art: its own three passes through a low resolution picture (the painted look keeps the composite)
	if (settings.pixelSize > 1 && settings.paintRadius <= 0 && !isClassicPixelLook)
	{
		DrawPixelArt(settings, bloom);
		rendererDevice.SetDepthTest(true);
		return;
	}

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
	compositeShader->SetUniform1f("impact", CurrentImpact());

	compositeShader->TrySetUniform1f("pixelSize", (float)glm::max(settings.pixelSize, 1));
	compositeShader->TrySetUniform1f("paletteLevels", (float)glm::max(settings.paletteLevels, 0));
	compositeShader->TrySetUniform1f("ditherStrength", settings.ditherStrength);
	compositeShader->TrySetUniform1f("outlineStrength", settings.outlineStrength);
	compositeShader->TrySetUniform3f("outlineColor", settings.outlineColor);
	compositeShader->TrySetUniform1f("outlineFog", settings.outlineFog);
	compositeShader->TrySetUniform4f("depthFromProjection", settings.depthFromProjection.x, settings.depthFromProjection.y, 0.0f, 0.0f);
	compositeShader->TrySetUniform1i("paintRadius", glm::clamp(settings.paintRadius, 0, 8));
	compositeShader->TrySetUniform1f("saturation", settings.saturation);
	compositeShader->TrySetUniform1f("contrast", settings.contrast);
	compositeShader->TrySetUniform3f("colorTint", settings.colorTint);
	compositeShader->TrySetUniform1f("vignette", settings.vignette);
	if (settings.outlineStrength > 0.0f)
	{
		sceneTarget->BindDepthTexture(DepthSlot);
		compositeShader->TrySetUniform1i("sceneDepth", DepthSlot);
	}
	DrawScreenQuad();

	rendererDevice.SetDepthTest(true);
}

void PostProcessing::DrawPixelArt(const PostProcessSettings& settings, int bloom)
{
	const FramebufferSpecification& scene = sceneTarget->GetSpecification();
	const int size   = glm::max(settings.pixelSize, 1);
	const int width  = glm::max((scene.width  + size - 1) / size, 1);
	const int height = glm::max((scene.height + size - 1) / size, 1);
	for (auto& target : pixelTargets)
		ResizeOrCreate(target, width, height, false);

	Shader& shader = *pixelArtShader;
	shader.Bind();
	shader.TrySetUniform1f("pixelSize", (float)size);

	// 0. Down: one surface per block, tone mapped, graded, brought to the palette's ramps
	{
		ScopedFramebufferBind bind(*pixelTargets[0]);
		shader.TrySetUniform1i("pass", 0);
		sceneTarget->BindColorTexture(0, SceneSlot);
		shader.TrySetUniform1i("scene", SceneSlot);
		sceneTarget->BindDepthTexture(DepthSlot);
		shader.TrySetUniform1i("sceneDepth", DepthSlot);
		shader.TrySetUniformBool("hasBloom", bloom >= 0);
		if (bloom >= 0)
		{
			bloomTargets[bloom]->BindColorTexture(0, BloomSlot);
			shader.TrySetUniform1i("bloom", BloomSlot);
			shader.TrySetUniform1f("bloomIntensity", settings.bloomIntensity);
		}
		shader.TrySetUniform1f("exposure", settings.exposure);
		shader.TrySetUniform1f("impact", CurrentImpact());
		shader.TrySetUniform4f("depthFromProjection", settings.depthFromProjection.x, settings.depthFromProjection.y, 0.0f, 0.0f);
		shader.TrySetUniform1f("saturation", settings.saturation);
		shader.TrySetUniform1f("contrast", settings.contrast);
		shader.TrySetUniform3f("colorTint", settings.colorTint);
		shader.TrySetUniform1f("vignette", settings.vignette);
		shader.TrySetUniform1f("paletteLevels", (float)glm::max(settings.paletteLevels, 0));
		shader.TrySetUniform1f("hueShift", settings.hueShift);
		shader.TrySetUniform1f("shadowHue", settings.shadowHue);
		shader.TrySetUniform1f("lightHue", settings.lightHue);
		DrawScreenQuad();
	}

	// 1. Ink: lone pixels, one pixel outlines without doubles
	shader.TrySetUniform1f("outlineStrength", settings.outlineStrength);
	shader.TrySetUniform3f("outlineColor", settings.outlineColor);
	shader.TrySetUniform1f("outlineFog", settings.outlineFog);
	shader.TrySetUniformBool("cleanLines", settings.cleanLines);
	shader.TrySetUniformBool("removeOrphans", settings.removeOrphans);
	{
		ScopedFramebufferBind bind(*pixelTargets[1]);
		shader.TrySetUniform1i("pass", 1);
		pixelTargets[0]->BindColorTexture(0, SceneSlot);
		shader.TrySetUniform1i("pixels", SceneSlot);
		DrawScreenQuad();
	}

	// 2. Up: on the screen, each low pixel a square
	shader.TrySetUniform1i("pass", 2);
	pixelTargets[1]->BindColorTexture(0, SceneSlot);
	shader.TrySetUniform1i("pixels", SceneSlot);
	DrawScreenQuad();
}

void PostProcessing::Kick(float strength, float seconds)
{
	strength = glm::clamp(strength, 0.0f, 1.0f);
	if (strength < CurrentImpact())
		return;

	ImpactFrame& impact = GetImpact();
	impact.strength = strength;
	impact.seconds  = seconds;
	impact.start    = std::chrono::steady_clock::now();
}

void PostProcessing::KickFromHit(float cameraShake)
{
	if (hitKickPerShake > 0.0f)
		Kick(cameraShake * hitKickPerShake);
}

}
