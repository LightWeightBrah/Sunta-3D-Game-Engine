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

void DebugRenderer::Init()
{
	RendererDevice& rendererDevice = Renderer::GetDevice();

	vertexArray = rendererDevice.CreateVertexArray();

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

}

void DebugRenderer::DrawBoxWireframe(const std::array<glm::vec3, BOX_CORNER_COUNT>& corners, const glm::vec3& color, const SceneData& sceneData)
{
	vertexBuffer->UpdateDynamicData(corners.data(), sizeof(glm::vec3) * BOX_CORNER_COUNT);

	debugShader->Bind();
	debugShader->SetUniformMatrix4fv("view",       sceneData.viewMatrix);
	debugShader->SetUniformMatrix4fv("projection", sceneData.projectionMatrix);
	debugShader->SetUniform3f("lineColor", color);
	
	auto& rendererDevice = Renderer::GetDevice();
	rendererDevice.SetLineWidth(gizmosLineWidth);
	rendererDevice.DrawLines(*vertexArray);
}

}