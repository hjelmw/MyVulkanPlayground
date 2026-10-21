#include "PostProcessNode.hpp"
#include "Utils/Pipeline.hpp"
#include "Utils/BindingTable.hpp"

namespace NVulkanEngine
{
	void CPostProcessNode::Init(SRuntimeContext* context, SGraphicsManagers* managers)
	{
		const SRenderResource sceneColorAttachment = managers->m_ResourceManager->GetRenderResource(EResourceIndices::SceneColor);

		m_PostProcessTable = new CBindingTable();
		m_PostProcessTable->AddStorageImageBinding(0, VK_SHADER_STAGE_COMPUTE_BIT, sceneColorAttachment.m_ImageView);
		m_PostProcessTable->CreateBindings(context);

		m_PostProcessPipeline = new CPipeline(EPipelineType::COMPUTE);
		m_PostProcessPipeline->SetDebugName("Post Process");
		m_PostProcessPipeline->SetComputeShader("shaders/postprocess.comp.spv");
		m_PostProcessPipeline->CreatePipeline(context, m_PostProcessTable->GetDescriptorSetLayout());
	}

	void CPostProcessNode::Draw(SRuntimeContext* context, SGraphicsManagers* managers, VkCommandBuffer commandBuffer)
	{
		// Writinh to storage images which have to be in the general layout
		managers->m_ResourceManager->TransitionResource(commandBuffer, EResourceIndices::SceneColor, VK_ATTACHMENT_LOAD_OP_LOAD, VK_IMAGE_LAYOUT_GENERAL);

		const float markerColor[4] = { 0.2f, 0.6f, 1.0f, 1.0f };
		BeginMarker(context->m_VulkanInstance, commandBuffer, "Post Process", markerColor);

		m_PostProcessPipeline->BindPipeline(commandBuffer);
		m_PostProcessTable->BindTable(context, commandBuffer, m_PostProcessPipeline->GetPipelineLayout(), m_PostProcessPipeline->GetBindPoint());

		// Round up so the work groups cover the entire image
		const VkExtent2D resolution = context->m_RenderResolution;
		const uint32_t groupCountX = (resolution.width  + s_WorkGroupSize - 1) / s_WorkGroupSize;
		const uint32_t groupCountY = (resolution.height + s_WorkGroupSize - 1) / s_WorkGroupSize;
		vkCmdDispatch(commandBuffer, groupCountX, groupCountY, 1);

		EndMarker(context->m_VulkanInstance, commandBuffer);
	}

	void CPostProcessNode::Cleanup(SRuntimeContext* context)
	{
		m_PostProcessTable->Cleanup(context);
		m_PostProcessPipeline->Cleanup(context);

		delete m_PostProcessTable;
		delete m_PostProcessPipeline;
	}
}
