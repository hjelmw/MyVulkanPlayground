#pragma once


#include <array>
#include <cassert>
#include <string>
#include <vector>

#include <GraphicsContext.hpp>
#include <VulkanGraphicsEngineUtils.hpp>

#include "RenderResources.hpp"

/*
	resource manager holds all the possible render to textures and keeps track of their states for you.
	Render resources are keyed by tag type, see RenderResources.hpp
*/

enum class EBufferIndices : uint32_t
{
	Geometry     = 0,
	Shadows      = 1,
	Atmospherics = 2,
	Terrain      = 3,
	DebugLines   = 4,
	Count        = 5,
};

namespace NVulkanEngine
{
	inline uint32_t g_NextResourceId = 0;
	template<class TRenderResource>
	uint32_t ResourceId()
	{
		// Static instance for each template class
		static const uint32_t id = g_NextResourceId++;
		return id;
	}

	class CResourceManager
	{
	public:
		// Upper bound used to size ImGui descriptor pools and the texture list
		static constexpr uint32_t m_MaxRenderResources = 32;

		CResourceManager(VkInstance vulkanInstance);
		~CResourceManager() = default;

		template<class TRenderResource>
		SRenderResource Add(SRuntimeContext* context,VkSampler sampler,uint32_t width,uint32_t height,VkFormat formatOverride = VK_FORMAT_UNDEFINED)
		{
			static_assert(CRenderResourceTag<TRenderResource>, "Resource tag needs m_DebugName, m_Format, m_Usage and m_InitialState, see RenderResources.hpp");

			const VkFormat format = formatOverride != VK_FORMAT_UNDEFINED ? formatOverride : TRenderResource::m_Format;
			return AddRenderResource(context, ResourceId<TRenderResource>(), TRenderResource::m_DebugName, sampler, VK_SHADER_STAGE_FRAGMENT_BIT, format, TRenderResource::m_Usage, TRenderResource::m_InitialState, width, height);
		}

		// Returns the resource if it has been created. You cannot modify this
		template<class TRenderResource>
		const SRenderResource& Get() const
		{
			return GetRenderResource(ResourceId<TRenderResource>());
		}

		// Returns the resource in state ready for rendering
		template<class TResource>
		SRenderResource TransitionResource(VkCommandBuffer commandBuffer, VkAttachmentLoadOp loadOperation, VkImageLayout wantedState)
		{
			return TransitionResource(commandBuffer, ResourceId<TRenderResource>(), loadOperation, wantedState);
		}

		SRenderResource AddRenderResource(
			SRuntimeContext*      context,
			uint32_t              resourceId,
			const std::string     debugName,
			VkSampler             sampler,
			VkShaderStageFlagBits shaderStageUsageFlags,
			VkFormat              format,
			VkImageUsageFlags     usage,
			VkImageLayout         imageLayout,
			uint32_t              width,
			uint32_t              height);

		const SRenderResource& GetRenderResource(uint32_t resourceId) const;
		const std::vector<SRenderResource>& GetRenderResources() const { return m_RenderResources; }

		SRenderResource TransitionResource(VkCommandBuffer commandBuffer, uint32_t resourceId, VkAttachmentLoadOp loadOperation, VkImageLayout wantedState);

		SUniformBufferResource AddUniformBuffer(
			SRuntimeContext* context,
			const std::string& debugName,
			EBufferIndices     uniformBufferIndex,
			VkDeviceSize       uniformBufferSize);

		// Returns the buffer if it has been created. You cannot modify this
		const SUniformBufferResource GetBufferResource(EBufferIndices bufferIndex);

		const std::array<SUniformBufferResource, (uint32_t)EBufferIndices::Count> GetBufferResources();

		void Cleanup(SRuntimeContext* context);
	private:
		void DestroyRenderResource(SRuntimeContext* context, SRenderResource& resource);

		std::vector<SRenderResource> m_RenderResources; // Indexed by ResourceId<Tag>()
		std::array<SUniformBufferResource, (uint32_t)EBufferIndices::Count>   m_BufferResources = {};

		// To mark attachments with debug names
		PFN_vkSetDebugUtilsObjectNameEXT m_VkSetDebugUtilsObjectNameEXT = nullptr;
	};
};
