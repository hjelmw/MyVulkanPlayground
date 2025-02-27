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
		VkFormat sceneColorFormat = managers->m_AttachmentManager->GetAttachment(EAttachmentIndices::SceneColor).m_Format;

		m_DebugUniformBuffer = CreateUniformBuffer(context, m_DebuguniformBufferMemory, sizeof(SDebugUniformUniformBuffer));

		m_DebugTable = new CBindingTable();
		m_DebugTable->AddUniformBufferBinding(0, VK_SHADER_STAGE_VERTEX_BIT, m_DebugUniformBuffer, sizeof(SDebugUniformUniformBuffer));
		m_DebugTable->CreateBindings(context);

		m_DebugPipeline = new CPipeline();
		m_DebugPipeline->SetVertexShader("shaders/debug.vert.spv");
		m_DebugPipeline->SetFragmentShader("shaders/debug.frag.spv");
		m_DebugPipeline->SetPrimitiveTopology(VK_PRIMITIVE_TOPOLOGY_LINE_LIST);
		m_DebugPipeline->SetCullingMode(VK_CULL_MODE_NONE);
		m_DebugPipeline->SetVertexInput(sizeof(SDebugVertexLine), VK_VERTEX_INPUT_RATE_VERTEX);
		m_DebugPipeline->AddVertexAttribute(0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(SDebugVertexLine, m_Position));
		m_DebugPipeline->AddVertexAttribute(1, VK_FORMAT_R32G32B32_SFLOAT, offsetof(SDebugVertexLine, m_Color));
		m_DebugPipeline->AddColorAttachment(sceneColorFormat);
		m_DebugPipeline->CreatePipeline(context, m_DebugTable->GetDescriptorSetLayout());
	}

	void CDebugNode::UpdateDebugUniformBuffer(CGraphicsContext* context, SGraphicsManagers* managers)
	{

		glm::mat4 cameraLookAt = managers->m_InputManager->GetCamera()->GetLookAtMatrix();
		glm::mat4 cameraProj = managers->m_InputManager->GetCamera()->GetProjectionMatrix();
		glm::mat4 cameraViewProj = cameraProj * cameraLookAt;
		SDebugUniformUniformBuffer debugUniformConstants{};
		debugUniformConstants.m_ViewProjectionMatrix = cameraViewProj;

		void* data;
		vkMapMemory(context->GetLogicalDevice(), m_DebuguniformBufferMemory, 0, sizeof(SDebugUniformUniformBuffer), 0, &data);
		memcpy(data, &debugUniformConstants, sizeof(SDebugUniformUniformBuffer));
		vkUnmapMemory(context->GetLogicalDevice(), m_DebuguniformBufferMemory);

	}

	void CDebugNode::Draw(CGraphicsContext* context, SGraphicsManagers* managers, VkCommandBuffer commandBuffer)
	{
		CDebugManager* debugManager = managers->m_DebugManager;
		const uint32_t numDebugLines = debugManager->GetNumDebugLines();

		if (numDebugLines == 0)
			return;

		CAttachmentManager* attachmentManager = managers->m_AttachmentManager;

		SRenderAttachment sceneColorAttachment = attachmentManager->TransitionAttachment(commandBuffer, EAttachmentIndices::SceneColor, VK_ATTACHMENT_LOAD_OP_LOAD, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

		BeginRendering("Debug Rendering", context, commandBuffer, { sceneColorAttachment });

		m_DebugPipeline->BindPipeline(commandBuffer);
		m_DebugTable->BindTable(context, commandBuffer, m_DebugPipeline->GetPipelineLayout());

		VkBuffer debugLinesVertexBuffers[] = { debugManager->GetDebugLinesVertexBuffer() };
		VkDeviceSize vertexOffsets[] = { 0 };
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, debugLinesVertexBuffers, vertexOffsets);

		vkCmdSetLineWidth(commandBuffer, 4.0f);

		UpdateDebugUniformBuffer(context, managers);

		vkCmdDraw(commandBuffer, numDebugLines, 1, 0, 0);

		EndRendering(context, commandBuffer);
	}

	void CDebugNode::Cleanup(CGraphicsContext* context)
	{
		vkDestroyBuffer(context->GetLogicalDevice(), m_DebugUniformBuffer, nullptr);
		vkFreeMemory(context->GetLogicalDevice(), m_DebuguniformBufferMemory, nullptr);

		m_DebugPipeline->Cleanup(context);
		m_DebugTable->Cleanup(context);
	}

};