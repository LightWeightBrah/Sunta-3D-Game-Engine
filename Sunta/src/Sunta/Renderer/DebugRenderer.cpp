#include "Core/SuntaPreCompiled.h"
#include "DebugRenderer.h"

#include "Renderer.h"
#include "RendererDevice.h"
#include "VertexArray.h"
#include "VertexBuffer.h"
#include "ElementBuffer.h"
#include "Shader.h"
#include "VertexLayouts.h"
#include "Scene/SceneData.h"
#include "Core/VirtualFileSystem.h"
#include "ScopedVertexArrayBind.h"

namespace Sunta
{

// box has 12 edges, each needs 2 indices (start corner, end corner)
constexpr unsigned int BOX_EDGE_COUNT = 12;
constexpr unsigned int INDICES_PER_EDGE = 2;
constexpr unsigned int BOX_EDGE_INDICES_COUNT = BOX_EDGE_COUNT * INDICES_PER_EDGE; // 24

static constexpr unsigned int BOX_WIREFRAME_EDGE_INDICES[BOX_EDGE_INDICES_COUNT]
{
	// bottom face ring
	0,1,  1,2,  2,3,  3,0,
	// top face ring
	4,5,  5,6,  6,7,  7,4,
	// vertical edges: bottom corner straight up to top corner
	0,4,  1,5,  2,6,  3,7
};

// a line has a start and an end, one index for each
constexpr unsigned int LINE_VERTEX_COUNT = 2;
static constexpr unsigned int LINE_INDICES[LINE_VERTEX_COUNT] = { 0, 1 };

void DebugRenderer::Init()
{
	RendererDevice& rendererDevice = Renderer::GetDevice();

	vertexArray = rendererDevice.CreateVertexArray();
	ScopedVertexArrayBind vertexArrayGuard(*vertexArray);

	// No inital data, cause every corner gets overwritten every draw call
	BufferDescriptor vertexBufferDescriptor;
	vertexBufferDescriptor.data = nullptr;
	vertexBufferDescriptor.size = sizeof(glm::vec3) * BOX_CORNER_COUNT;
	vertexBufferDescriptor.usage = BufferUsage::Dynamic;

	vertexBuffer = rendererDevice.CreateVertexBuffer(vertexBufferDescriptor);
	vertexBuffer->SetLayout(VertexLayouts::GetPositionOnlyLayout());

	// 12 edges are always same shape, indicies (only corner positions move)
	// so element buffer doesn't needs to change
	BufferDescriptor elementBufferDescriptor;
	elementBufferDescriptor.data = BOX_WIREFRAME_EDGE_INDICES;
	elementBufferDescriptor.size = sizeof(BOX_WIREFRAME_EDGE_INDICES);
	elementBufferDescriptor.usage = BufferUsage::Static;

	auto elementBuffer = rendererDevice.CreateElementBuffer(elementBufferDescriptor);

	vertexArray->AddVertexBuffer(vertexBuffer);
	vertexArray->SetElementBuffer(elementBuffer);

	debugShader = rendererDevice.CreateShader(VirtualFileSystem::Resolve("@engine/Shaders/DebugCollider.shader"));

	CreateLineBuffers();
}

void DebugRenderer::CreateLineBuffers()
{
	RendererDevice& rendererDevice = Renderer::GetDevice();

	lineVertexArray = rendererDevice.CreateVertexArray();
	ScopedVertexArrayBind vertexArrayGuard(*lineVertexArray);

	BufferDescriptor vertexBufferDescriptor;
	vertexBufferDescriptor.data = nullptr;
	vertexBufferDescriptor.size = sizeof(glm::vec3) * LINE_VERTEX_COUNT;
	vertexBufferDescriptor.usage = BufferUsage::Dynamic;

	lineVertexBuffer = rendererDevice.CreateVertexBuffer(vertexBufferDescriptor);
	lineVertexBuffer->SetLayout(VertexLayouts::GetPositionOnlyLayout());

	BufferDescriptor elementBufferDescriptor;
	elementBufferDescriptor.data = LINE_INDICES;
	elementBufferDescriptor.size = sizeof(LINE_INDICES);
	elementBufferDescriptor.usage = BufferUsage::Static;

	auto elementBuffer = rendererDevice.CreateElementBuffer(elementBufferDescriptor);

	lineVertexArray->AddVertexBuffer(lineVertexBuffer);
	lineVertexArray->SetElementBuffer(elementBuffer);
}

void DebugRenderer::DrawBoxWireframe(const std::array<glm::vec3, BOX_CORNER_COUNT>& corners, const glm::vec3& color, const SceneData& sceneData)
{
	vertexBuffer->UpdateDynamicData(corners.data(), sizeof(glm::vec3) * BOX_CORNER_COUNT);

	DrawLinesWithColor(*vertexArray, color, sceneData);
}

void DebugRenderer::DrawLine(const glm::vec3& from, const glm::vec3& to, const glm::vec3& color, const SceneData& sceneData)
{
	const glm::vec3 points[LINE_VERTEX_COUNT] = { from, to };
	lineVertexBuffer->UpdateDynamicData(points, sizeof(points));

	DrawLinesWithColor(*lineVertexArray, color, sceneData);
}

void DebugRenderer::DrawCross(const glm::vec3& center, float halfSize, const glm::vec3& color, const SceneData& sceneData)
{
	DrawLine(center - glm::vec3(halfSize, 0.0f, 0.0f), center + glm::vec3(halfSize, 0.0f, 0.0f), color, sceneData);
	DrawLine(center - glm::vec3(0.0f, halfSize, 0.0f), center + glm::vec3(0.0f, halfSize, 0.0f), color, sceneData);
	DrawLine(center - glm::vec3(0.0f, 0.0f, halfSize), center + glm::vec3(0.0f, 0.0f, halfSize), color, sceneData);
}

void DebugRenderer::DrawLinesWithColor(const VertexArray& lines, const glm::vec3& color, const SceneData& sceneData)
{
	debugShader->Bind();
	debugShader->SetUniformMatrix4fv("view",       sceneData.viewMatrix);
	debugShader->SetUniformMatrix4fv("projection", sceneData.projectionMatrix);
	debugShader->SetUniform3f("lineColor", color);

	auto& rendererDevice = Renderer::GetDevice();
	rendererDevice.SetLineWidth(gizmosLineWidth);
	rendererDevice.DrawLines(lines);
}

}