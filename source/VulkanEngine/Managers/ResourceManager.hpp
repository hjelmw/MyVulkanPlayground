#pragma once

#include <string>
#include <map>

#include <GraphicsContext.hpp>
#include <VulkanGraphicsEngineUtils.hpp>
#include <BindlessTable.hpp>

/*
	Attachment manager holds all the rendertargets and keeps track of their states for you
*/

enum class EResourceIndices : uint32_t
{
	Positions          = 0,
	Normals            = 1,
	Albedo             = 2,
	Depth              = 3,
	ShadowMap          = 4,
	SceneColor         = 5,
	AtmosphericsSkyBox = 6,
	Count              = 7
};

enum class EBufferIndices : uint32_t
{
	Count = 0,
};

namespace NVulkanEngine
{
	class CResourceManager
	{
	public:
		CResourceManager(VkInstance vulkanInstance);
		~CResourceManager() = default;

		SRenderResource AddResource(
			CGraphicsContext*     context,
			const std::string     debugName,
			EResourceIndices    attachmentIndex,
			VkSampler             sampler,
			VkShaderStageFlagBits shaderStageUsageFlags,
			VkFormat              format,
			VkImageUsageFlags     usage,
			VkImageLayout         imageLayout,
			uint32_t              width,
			uint32_t              height);

		// Returns the attachment if it has been created. You cannot modify this
		const SRenderResource GetResource(EResourceIndices index);
		const std::array<SRenderResource, (uint32_t)EResourceIndices::Count> GetResources();

		// Returns the attachment in state ready for rendering
		SRenderResource TransitionResource(VkCommandBuffer commandBuffer, EResourceIndices index,VkAttachmentLoadOp loadOperation, VkImageLayout wantedState);

		void Cleanup(CGraphicsContext* context);
	private:
		std::array<SRenderResource, (uint32_t)EResourceIndices::Count> m_RenderAttachments = {};

		CBindlessTable* m_BindlessTable = nullptr;

		// To mark attachments with debug names
		PFN_vkSetDebugUtilsObjectNameEXT m_VkSetDebugUtilsObjectNameEXT = nullptr;
	};
};