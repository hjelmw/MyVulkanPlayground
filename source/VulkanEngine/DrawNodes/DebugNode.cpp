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
		std::vector<SDebugLine> debugLines = {};

		SDebugLine debugLine1{};
		debugLine1.m_Position  = glm::vec3(0.0f, 200.0f, 0.0f);
		debugLine1.m_Color     = glm::vec3(1.0f, 0.0f, 0.0f);

		SDebugLine debugLine2{};
		debugLine2.m_Position = glm::vec3(0.0f, 400.0f, 0.0f);
		debugLine2.m_Color = glm::vec3(1.0f, 0.0f, 0.0f);

		debugLines.push_back(debugLine1);
		debugLines.push_back(debugLine2);

		VkDeviceSize bufferSize = sizeof(debugLines[0]) * debugLines.size();

		/* CPU side Staging buffer */
		VkDeviceMemory stagingBufferMemory;

		VkBuffer stagingBuffer = CreateBuffer(
			context,
			stagingBufferMemory,
			bufferSize,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

		void* data;
		vkMapMemory(context->GetLogicalDevice(), stagingBufferMemory, 0, bufferSize, 0, &data);
		memcpy(data, debugLines.data(), (size_t)bufferSize);
		vkUnmapMemory(context->GetLogicalDevice(), stagingBufferMemory);

		/* GPU Side */
		m_VertexBuffer = CreateBuffer(
			context,
			m_VertexBufferMemory,
			bufferSize,
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		/* Copy staging buffer to device*/
		CopyBuffer(context, stagingBuffer, m_VertexBuffer, bufferSize);

		vkDestroyBuffer(context->GetLogicalDevice(), stagingBuffer, nullptr);
		vkFreeMemory(context->GetLogicalDevice(), stagingBufferMemory, nullptr);

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
		m_DebugPipeline->SetVertexInput(sizeof(SDebugLine), VK_VERTEX_INPUT_RATE_VERTEX);
		m_DebugPipeline->AddVertexAttribute(0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(SDebugLine, m_Position));
		m_DebugPipeline->AddVertexAttribute(1, VK_FORMAT_R32G32B32_SFLOAT, offsetof(SDebugLine, m_Color));
		m_DebugPipeline->AddColorAttachment(sceneColorFormat);
		m_DebugPipeline->CreatePipeline(context, m_DebugTable->GetDescriptorSetLayout());
	}

	void CDebugNode::Draw(CGraphicsContext* context, SGraphicsManagers* managers, VkCommandBuffer commandBuffer)
	{
		CAttachmentManager* attachmentManager = managers->m_AttachmentManager;

		SRenderAttachment sceneColorAttachment = attachmentManager->TransitionAttachment(commandBuffer, EAttachmentIndices::SceneColor, VK_ATTACHMENT_LOAD_OP_LOAD, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

		BeginRendering("Debug Rendering", context, commandBuffer, { sceneColorAttachment });

		m_DebugPipeline->BindPipeline(commandBuffer);
		m_DebugTable->BindTable(context, commandBuffer, m_DebugPipeline->GetPipelineLayout());

		VkBuffer vertexBuffers[] = { m_VertexBuffer };

		VkDeviceSize vertexOffsets[] = { 0 };
		VkDeviceSize indexOfssets = 0;

		vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, vertexOffsets);

		glm::mat4 cameraLookAt = managers->m_InputManager->GetCamera()->GetLookAtMatrix();
		glm::mat4 cameraProj = managers->m_InputManager->GetCamera()->GetProjectionMatrix();
		glm::mat4 cameraViewProj = cameraProj * cameraLookAt;
		SDebugUniformUniformBuffer debugUniformConstants{};
		debugUniformConstants.m_ViewProjectionMatrix = cameraViewProj;

		void* data;
		vkMapMemory(context->GetLogicalDevice(), m_DebuguniformBufferMemory, 0, sizeof(SDebugUniformUniformBuffer), 0, &data);
		memcpy(data, &debugUniformConstants, sizeof(SDebugUniformUniformBuffer));
		vkUnmapMemory(context->GetLogicalDevice(), m_DebuguniformBufferMemory);

		vkCmdSetLineWidth(commandBuffer, 4.0f);

		vkCmdDraw(commandBuffer, 2, 1, 0, 0);

		EndRendering(context, commandBuffer);
	}

	void CDebugNode::Cleanup(CGraphicsContext* context)
	{

	}

};