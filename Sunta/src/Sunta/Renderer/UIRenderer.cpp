#include "Core/SuntaPreCompiled.h"
#include "UIRenderer.h"

#include <glm/gtc/matrix_transform.hpp>

#include "ElementBuffer.h"
#include "Font.h"
#include "Renderer.h"
#include "RendererDevice.h"
#include "ScopedVertexArrayBind.h"
#include "Shader.h"
#include "Texture.h"
#include "VertexArray.h"
#include "VertexBuffer.h"
#include "VertexLayouts.h"
#include "Core/VirtualFileSystem.h"

namespace Sunta
{

namespace
{

constexpr const char* ShaderPath = "@engine/Shaders/UI.shader";
constexpr const char* FontPath   = "@engine/Fonts/Crimson_Text/CrimsonText-SemiBold.ttf";

// The letters are drawn into the font texture at this size, smaller and bigger text is scaled from it
constexpr float FontBakedPixelHeight = 72.0f;

// The shadow under text: how far down and right it is (part of the letter height) and how dark
constexpr float TextShadowOffset = 0.06f;
const glm::vec4 TextShadowColor  = glm::vec4(0.0f, 0.0f, 0.0f, 0.75f);

const glm::vec4 WholeTexture = glm::vec4(0.0f, 0.0f, 1.0f, 1.0f);

// A square from 0,0 to 1,1 that the shader stretches over each rectangle
const glm::vec3 QuadCorners[] =
{
	{ 0.0f, 0.0f, 0.0f },
	{ 1.0f, 0.0f, 0.0f },
	{ 1.0f, 1.0f, 0.0f },
	{ 0.0f, 1.0f, 0.0f }
};

const unsigned int QuadIndices[] = { 0, 1, 2, 2, 3, 0 };

}

void UIRenderer::Begin(const glm::vec2& newScreenSize)
{
	EnsureInitialized();

	screenSize = newScreenSize;

	RendererDevice& rendererDevice = Renderer::GetDevice();
	rendererDevice.SetDepthTest(false);
	rendererDevice.SetAlphaBlending(true);

	shader->Bind();
	shader->SetUniformMatrix4fv("projection", glm::ortho(0.0f, screenSize.x, screenSize.y, 0.0f));
}

void UIRenderer::End()
{
	RendererDevice& rendererDevice = Renderer::GetDevice();
	rendererDevice.SetAlphaBlending(false);
	rendererDevice.SetDepthTest(true);
}

void UIRenderer::EnsureInitialized()
{
	if (shader)
		return;

	RendererDevice& rendererDevice = Renderer::GetDevice();

	CreateQuad();
	shader = rendererDevice.CreateShader(VirtualFileSystem::Resolve(ShaderPath));

	font = std::make_shared<Font>();
	font->TryLoad(rendererDevice, VirtualFileSystem::Resolve(FontPath), FontBakedPixelHeight);
}

void UIRenderer::CreateQuad()
{
	RendererDevice& rendererDevice = Renderer::GetDevice();

	quad = rendererDevice.CreateVertexArray();
	ScopedVertexArrayBind vertexArrayGuard(*quad);

	BufferDescriptor vertexBufferDescriptor;
	vertexBufferDescriptor.data = QuadCorners;
	vertexBufferDescriptor.size = sizeof(QuadCorners);

	auto vertexBuffer = rendererDevice.CreateVertexBuffer(vertexBufferDescriptor);
	vertexBuffer->SetLayout(VertexLayouts::GetPositionOnlyLayout());

	BufferDescriptor elementBufferDescriptor;
	elementBufferDescriptor.data = QuadIndices;
	elementBufferDescriptor.size = sizeof(QuadIndices);

	quad->AddVertexBuffer(vertexBuffer);
	quad->SetElementBuffer(rendererDevice.CreateElementBuffer(elementBufferDescriptor));
}

void UIRenderer::DrawQuad(const glm::vec2& topLeft, const glm::vec2& size, const glm::vec4& color, const Texture* texture, const glm::vec4& textureArea)
{
	shader->SetUniform4f("rectangle", topLeft.x, topLeft.y, size.x, size.y);
	shader->SetUniform4f("textureArea", textureArea.x, textureArea.y, textureArea.z, textureArea.w);
	shader->SetUniform4f("color", color.r, color.g, color.b, color.a);
	shader->SetUniformBool("hasTexture", texture != nullptr);

	if (texture)
	{
		texture->Bind(0);
		shader->SetUniform1i("image", 0);
	}

	Renderer::GetDevice().DrawElements(*quad);
}

void UIRenderer::DrawRectangle(const glm::vec2& topLeft, const glm::vec2& size, const glm::vec4& color)
{
	DrawQuad(topLeft, size, color, nullptr, WholeTexture);
}

void UIRenderer::DrawImage(const glm::vec2& topLeft, const glm::vec2& size, const Texture& texture)
{
	DrawQuad(topLeft, size, glm::vec4(1.0f), &texture, WholeTexture);
}

void UIRenderer::DrawRectangleOutline(const glm::vec2& topLeft, const glm::vec2& size, float thickness, const glm::vec4& color)
{
	DrawRectangle(topLeft, glm::vec2(size.x, thickness), color);
	DrawRectangle(glm::vec2(topLeft.x, topLeft.y + size.y - thickness), glm::vec2(size.x, thickness), color);
	DrawRectangle(topLeft, glm::vec2(thickness, size.y), color);
	DrawRectangle(glm::vec2(topLeft.x + size.x - thickness, topLeft.y), glm::vec2(thickness, size.y), color);
}

void UIRenderer::DrawLetters(const std::string& text, const glm::vec2& topLeft, float pixelHeight, const glm::vec4& color)
{
	for (const Font::LetterQuad& letter : font->LayOut(text, topLeft, pixelHeight))
		DrawQuad(letter.position, letter.size, color, &font->GetTexture(), letter.textureArea);
}

void UIRenderer::DrawTextAt(const std::string& text, const glm::vec2& topLeft, float pixelHeight, const glm::vec4& color)
{
	if (!font->IsLoaded())
		return;

	const glm::vec2 shadowOffset = glm::vec2(pixelHeight * TextShadowOffset);
	const glm::vec4 shadowColor  = glm::vec4(glm::vec3(TextShadowColor), TextShadowColor.a * color.a);

	DrawLetters(text, topLeft + shadowOffset, pixelHeight, shadowColor);
	DrawLetters(text, topLeft, pixelHeight, color);
}

void UIRenderer::DrawTextCentered(const std::string& text, const glm::vec2& center, float pixelHeight, const glm::vec4& color)
{
	const glm::vec2 topLeft = center - glm::vec2(MeasureText(text, pixelHeight), pixelHeight) * 0.5f;
	DrawTextAt(text, topLeft, pixelHeight, color);
}

float UIRenderer::MeasureText(const std::string& text, float pixelHeight)
{
	return font && font->IsLoaded() ? font->MeasureWidth(text, pixelHeight) : 0.0f;
}

bool UIRenderer::TryProjectToScreen(const glm::vec3& worldPoint, const glm::mat4& viewProjection, glm::vec2& outPixel)
{
	const glm::vec4 clip = viewProjection * glm::vec4(worldPoint, 1.0f);
	if (clip.w <= 0.0f)
		return false;

	const glm::vec2 normalized = glm::vec2(clip) / clip.w; // -1..1, y up

	outPixel = glm::vec2((normalized.x * 0.5f + 0.5f) * screenSize.x, (0.5f - normalized.y * 0.5f) * screenSize.y);
	return true;
}

}
