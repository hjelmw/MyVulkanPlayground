#include "DebugNode.hpp"
#include <Managers/DebugManager.hpp>

namespace NVulkanEngine
{

	struct SDebugUniformUniformBuffer
	{
		glm::mat4 m_ViewProjectionMatrix = glm::identity<glm::mat4>();
	};

	void CDebugNode::Init(CGraphicsContext* context, SGraphicsManagers* managers)
	{
		VkFormat sceneColorFormat = managers->m_ResourceManager->GetRenderResource(EResourceIndices::SceneColor).m_Format;

		managers->m_ResourceManager->AddUniformBuffer(context, "DebugLines Uniforms", EBufferIndices::DebugLines, sizeof(SDebugUniformUniformBuffer));

		m_DebugPipeline = new CPipeline(EPipelineType::GRAPHICS);
		m_DebugPipeline->SetDebugName("Debug Lines");
		m_DebugPipeline->SetVertexShader("shaders/debug.vert.spv");
		m_DebugPipeline->SetFragmentShader("shaders/debug.frag.spv");
		m_DebugPipeline->SetPrimitiveTopology(VK_PRIMITIVE_TOPOLOGY_LINE_LIST);
		m_DebugPipeline->SetCullingMode(VK_CULL_MODE_NONE);
		m_DebugPipeline->SetVertexInput(sizeof(SDebugVertexLine), VK_VERTEX_INPUT_RATE_VERTEX);
		m_DebugPipeline->AddVertexAttribute(0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(SDebugVertexLine, m_Position));
		m_DebugPipeline->AddVertexAttribute(1, VK_FORMAT_R32G32B32_SFLOAT, offsetof(SDebugVertexLine, m_Color));
		m_DebugPipeline->AddColorAttachment(sceneColorFormat);
		managers->m_PipelineManager->RegisterPipeline(m_DebugPipeline);
	}

	void CDebugNode::UpdateBeforeDraw(VkDevice logicalDevice, SGraphicsManagers* managers)
	{
		SUniformBufferResource bufferResource = managers->m_ResourceManager->GetBufferResource(EBufferIndices::DebugLines);

		glm::mat4 cameraLookAt = managers->m_InputManager->GetCamera()->GetLookAtMatrix();
		glm::mat4 cameraProj = managers->m_InputManager->GetCamera()->GetProjectionMatrix();
		glm::mat4 cameraViewProj = cameraProj * cameraLookAt;

		SDebugUniformUniformBuffer debugUniformConstants{};
		debugUniformConstants.m_ViewProjectionMatrix = cameraViewProj;

		void* data;
		vkMapMemory(logicalDevice, bufferResource.m_Memory, 0, sizeof(SDebugUniformUniformBuffer), 0, &data);
		memcpy(data, &debugUniformConstants, sizeof(SDebugUniformUniformBuffer));
		vkUnmapMemory(logicalDevice, bufferResource.m_Memory);
	}

	void CDebugNode::Draw(CGraphicsContext* context, SGraphicsManagers* managers, VkCommandBuffer commandBuffer)
	{
		CDebugManager* debugManager = managers->m_DebugManager;
		const uint32_t numDebugLines = debugManager->GetNumDebugLines();

		if (numDebugLines == 0)
			return;

		CResourceManager* resourceManager = managers->m_ResourceManager;

		SRenderResource sceneColorAttachment = resourceManager->TransitionResource(commandBuffer, EResourceIndices::SceneColor, VK_ATTACHMENT_LOAD_OP_LOAD, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

		BeginRendering("Debug Rendering", context, commandBuffer, { sceneColorAttachment });

		m_DebugPipeline->BindPipeline(commandBuffer);

		VkBuffer debugLinesVertexBuffers[] = { debugManager->GetDebugLinesVertexBuffer() };
		VkDeviceSize vertexOffsets[] = { 0 };
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, debugLinesVertexBuffers, vertexOffsets);

		vkCmdSetLineWidth(commandBuffer, 4.0f);
		vkCmdDraw(commandBuffer, numDebugLines, 1, 0, 0);

		EndRendering(context, commandBuffer);
	}

	void CDebugNode::Cleanup(CGraphicsContext* context)
	{
		m_DebugPipeline->Cleanup(context);
	}

};