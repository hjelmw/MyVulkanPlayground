#include "ResourceManager.hpp"

#include <VulkanGraphicsEngineUtils.hpp>
#include <backends/imgui_impl_vulkan.h>

namespace NVulkanEngine
{
	CResourceManager::CResourceManager(VkInstance vulkanInstance)
	{
		m_VkSetDebugUtilsObjectNameEXT = (PFN_vkSetDebugUtilsObjectNameEXT)vkGetInstanceProcAddr(vulkanInstance, "vkSetDebugUtilsObjectNameEXT");

		m_BindlessTable = new CBindlessTable();
	}

	SRenderResource CResourceManager::AddResource(
		CGraphicsContext*     context,
		const std::string     debugName,
		EResourceIndices    attachmentIndex,
		VkSampler             sampler,
		VkShaderStageFlagBits shaderStageUsageFlags,
		VkFormat              format,
		VkImageUsageFlags     usage,
		VkImageLayout         imageLayout,
		uint32_t              width,
		uint32_t              height)
	{
		SRenderResource renderResource  = CreateRenderResource(context, format, usage, imageLayout, width, height);

		// ImGui allocates a descriptor with VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER where there are some image layouts that are not allowed
		VkImageLayout imGuiImageLayout = renderResource.m_CurrentImageLayout;
		if (usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)
		{
			imGuiImageLayout = VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL;
		}
		else if (usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)
		{
			imGuiImageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		}

		renderResource.m_ImguiDescriptor  = ImGui_ImplVulkan_AddTexture(sampler, renderResource.m_ImageView, imGuiImageLayout);

		// If no name is provided resource uses default name
		if (debugName.length() > 0)
		{
			memset(renderResource.m_DebugName, 0, 64);
			memcpy(renderResource.m_DebugName, debugName.c_str(), debugName.length());
		}
		if (m_VkSetDebugUtilsObjectNameEXT)
		{
			VkDebugUtilsObjectNameInfoEXT nameInfo = { VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT };
			nameInfo.objectType   = VK_OBJECT_TYPE_IMAGE;
			nameInfo.objectHandle = (uint64_t)renderResource.m_Image;
			nameInfo.pObjectName  = renderResource.m_DebugName;
			m_VkSetDebugUtilsObjectNameEXT(context->GetLogicalDevice(), &nameInfo);
		}

		// Add to the global table
		m_BindlessTable->AddSampledImageBinding((uint32_t)attachmentIndex, shaderStageUsageFlags, renderResource.m_ImageView, format, sampler);

		m_RenderAttachments[(uint32_t)attachmentIndex] = renderResource;
		return renderResource;
	}

	const SRenderResource CResourceManager::GetResource(const EResourceIndices index)
	{
		return m_RenderAttachments[(uint32_t)index];
	}

	const std::array<SRenderResource, (uint32_t)EResourceIndices::Count> CResourceManager::GetResources()
	{
		return m_RenderAttachments;
	}


	SRenderResource CResourceManager::TransitionResource(
		VkCommandBuffer          commandBuffer,
		const EResourceIndices index, 
		VkAttachmentLoadOp       loadOperation,
		VkImageLayout            wantedLayout)
	{
		SRenderResource& attachment = m_RenderAttachments[(uint32_t)index];

		if (wantedLayout != VK_IMAGE_LAYOUT_UNDEFINED && wantedLayout != attachment.m_CurrentImageLayout)
			TransitionImageLayout(commandBuffer, attachment, wantedLayout, 1);

		attachment.m_RenderAttachmentInfo.loadOp = loadOperation;

		return attachment;
	}


	void CResourceManager::Cleanup(CGraphicsContext* context)
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
