#include "ResourceManager.hpp"

#include <Core/VulkanGraphicsEngineUtils.hpp>
#include <backends/imgui_impl_vulkan.h>

namespace NVulkanEngine
{
	CResourceManager::CResourceManager(VkInstance vulkanInstance)
	{
		m_VkSetDebugUtilsObjectNameEXT = (PFN_vkSetDebugUtilsObjectNameEXT)vkGetInstanceProcAddr(vulkanInstance, "vkSetDebugUtilsObjectNameEXT");
	}

	SRenderResource CResourceManager::AddRenderResource(
		SRuntimeContext*      context,
		uint32_t              resourceId,
		const std::string     debugName,
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
			m_VkSetDebugUtilsObjectNameEXT(context->m_VulkanDevice, &nameInfo);
		}

		if (resourceId >= m_RenderResources.size())
			m_RenderResources.resize(resourceId + 1);

		m_RenderResources[resourceId] = renderResource;
		return renderResource;
	}

	SUniformBufferResource CResourceManager::AddUniformBuffer(
		SRuntimeContext*  context, 
		const std::string& debugName,
		EBufferIndices     uniformBufferIndex,
		VkDeviceSize       uniformBufferSize)
	{
		VkDeviceMemory uniformBufferMemory;
		VkBuffer uniformBuffer = CreateUniformBuffer(context, uniformBufferMemory, uniformBufferSize);

		SUniformBufferResource uniformBufferResource{};

		if (debugName.length() > 0)
		{
			memset(uniformBufferResource.m_DebugName, 0, 64);
			memcpy(uniformBufferResource.m_DebugName, debugName.c_str(), debugName.length());
		}

		uniformBufferResource.m_Buffer = uniformBuffer;
		uniformBufferResource.m_Memory = uniformBufferMemory;
		uniformBufferResource.m_Size   = (uint32_t)uniformBufferSize;

		m_BufferResources[(uint32_t) uniformBufferIndex] = uniformBufferResource;

		return uniformBufferResource;
	}

	const SRenderResource& CResourceManager::GetRenderResource(const uint32_t resourceId) const
	{
		assert(resourceId < m_RenderResources.size() && "Resource was never added. Forgot to call Add()?");
		return m_RenderResources[resourceId];
	}

	const SUniformBufferResource CResourceManager::GetBufferResource(const EBufferIndices index)
	{
		return m_BufferResources[(uint32_t)index];
	}

	const std::array<SUniformBufferResource, (uint32_t)EBufferIndices::Count> CResourceManager::GetBufferResources()
	{
		return m_BufferResources;
	}

	SRenderResource CResourceManager::TransitionResource(
		VkCommandBuffer    commandBuffer,
		const uint32_t     resourceId,
		VkAttachmentLoadOp loadOperation,
		VkImageLayout      wantedLayout)
	{
		assert(resourceId < m_RenderResources.size() && "Resource was never added");
		SRenderResource& attachment = m_RenderResources[resourceId];

		if (wantedLayout != VK_IMAGE_LAYOUT_UNDEFINED && wantedLayout != attachment.m_CurrentImageLayout)
			TransitionImageLayout(commandBuffer, attachment, wantedLayout, 1);

		attachment.m_RenderAttachmentInfo.loadOp = loadOperation;

		return attachment;
	}

	void CResourceManager::DestroyRenderResource(SRuntimeContext* context, SRenderResource& resource)
	{
		vkDestroyImageView(context->m_VulkanDevice, resource.m_ImageView, nullptr);
		vkDestroyImage(context->m_VulkanDevice, resource.m_Image, nullptr);
		vkFreeMemory(context->m_VulkanDevice, resource.m_Memory, nullptr);

		ImGui_ImplVulkan_RemoveTexture(resource.m_ImguiDescriptor);
		resource = {};
	}

	void CResourceManager::Cleanup(SRuntimeContext* context)
	{
		for (SRenderResource& resource : m_RenderResources)
			DestroyRenderResource(context, resource);
	}
};
