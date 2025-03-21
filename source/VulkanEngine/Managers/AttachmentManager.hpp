#pragma once

#include <string>
#include <map>

#include <GraphicsContext.hpp>
#include <VulkanGraphicsEngineUtils.hpp>

/*
	Attachment manager holds all the rendertargets and keeps track of their states for you
*/

enum class EAttachmentIndices : uint32_t
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

namespace NVulkanEngine
{
	class CAttachmentManager
	{
	public:
		CAttachmentManager(VkInstance vulkanInstance);
		~CAttachmentManager() = default;

		SRenderAttachment AddAttachment(
			CGraphicsContext*  context,
			const std::string  debugName,
			EAttachmentIndices attachmentIndex,
			VkSampler          sampler,
			VkFormat           format,
			VkImageUsageFlags  usage,
			VkImageLayout      imageLayout,
			uint32_t           width,
			uint32_t           height);

		// Returns the attachment if it has been created. You cannot modify this
		const SRenderAttachment GetAttachment(EAttachmentIndices index);
		const std::array<SRenderAttachment, (uint32_t)EAttachmentIndices::Count> GetAttachments();

		// Returns the attachment in state ready for rendering
		SRenderAttachment TransitionAttachment(VkCommandBuffer commandBuffer, EAttachmentIndices index,VkAttachmentLoadOp loadOperation, VkImageLayout wantedState);

		void Cleanup(CGraphicsContext* context);
	private:
		std::array<SRenderAttachment, (uint32_t)EAttachmentIndices::Count> m_RenderAttachments = {};

		// To mark attachments with debug names
		PFN_vkSetDebugUtilsObjectNameEXT m_VkSetDebugUtilsObjectNameEXT = nullptr;
	};
};