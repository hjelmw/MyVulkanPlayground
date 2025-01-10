#include "AttachmentManager.hpp"

#include <VulkanGraphicsEngineUtils.hpp>
#include <backends/imgui_impl_vulkan.h>

namespace NVulkanEngine
{
	CAttachmentManager::CAttachmentManager(VkInstance vulkanInstance)
	{
		m_VkSetDebugUtilsObjectNameEXT = (PFN_vkSetDebugUtilsObjectNameEXT)vkGetInstanceProcAddr(vulkanInstance, "vkSetDebugUtilsObjectNameEXT");
	}

	SRenderAttachment CAttachmentManager::AddAttachment(
		CGraphicsContext*  context,
		const std::string  debugName,
		EAttachmentIndices attachmentIndex,
		VkSampler          sampler,
		VkFormat           format,
		VkImageUsageFlags  usage,
		VkImageLayout      imageLayout,
		uint32_t           width,
		uint32_t           height)
	{
		SRenderAttachment attachment  = CreateRenderAttachment(context, format, usage, imageLayout, width, height);

		// ImGui allocates a descriptor with VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER where there are some image layouts that are not allowed
		VkImageLayout imGuiImageLayout = attachment.m_CurrentImageLayout;
		if (usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)
		{
			imGuiImageLayout = VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL;
		}
		else if (usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)
		{
			imGuiImageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		}

		attachment.m_ImguiDescriptor  = ImGui_ImplVulkan_AddTexture(sampler, attachment.m_ImageView, imGuiImageLayout);

		memcpy(attachment.m_DebugName, debugName.c_str(), debugName.length());
		if (m_VkSetDebugUtilsObjectNameEXT)
		{
			VkDebugUtilsObjectNameInfoEXT nameInfo = { VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT };
			nameInfo.objectType   = VK_OBJECT_TYPE_IMAGE;
			nameInfo.objectHandle = (uint64_t)attachment.m_Image;
			nameInfo.pObjectName  = attachment.m_DebugName;
			m_VkSetDebugUtilsObjectNameEXT(context->GetLogicalDevice(), &nameInfo);
		}

		m_RenderAttachments[(uint32_t)attachmentIndex] = attachment;
		return attachment;
	}

	const SRenderAttachment CAttachmentManager::GetAttachment(const EAttachmentIndices index)
	{
		return m_RenderAttachments[(uint32_t)index];
	}

	const std::array<SRenderAttachment, (uint32_t)EAttachmentIndices::Count> CAttachmentManager::GetAttachments()
	{
		return m_RenderAttachments;
	}


	SRenderAttachment CAttachmentManager::TransitionAttachment(
		VkCommandBuffer          commandBuffer,
		const EAttachmentIndices index, 
		VkAttachmentLoadOp       loadOperation,
		VkImageLayout            wantedLayout)
	{
		SRenderAttachment& attachment = m_RenderAttachments[(uint32_t)index];

		if (wantedLayout != VK_IMAGE_LAYOUT_UNDEFINED && wantedLayout != attachment.m_CurrentImageLayout)
			TransitionImageLayout(commandBuffer, attachment, wantedLayout, 1);

		attachment.m_RenderAttachmentInfo.loadOp = loadOperation;

		return attachment;
	}


	void CAttachmentManager::Cleanup(CGraphicsContext* context)
	{
		for (uint32_t i = 0; i < m_RenderAttachments.size(); i++)
		{
			vkDestroyImageView(context->GetLogicalDevice(), m_RenderAttachments[i].m_ImageView, nullptr);
			vkDestroyImage(context->GetLogicalDevice(), m_RenderAttachments[i].m_Image, nullptr);
			vkFreeMemory(context->GetLogicalDevice(), m_RenderAttachments[i].m_Memory, nullptr);

			ImGui_ImplVulkan_RemoveTexture(m_RenderAttachments[i].m_ImguiDescriptor);
			m_RenderAttachments[i] = {};			 
		}
	}
};
