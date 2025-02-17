#include "DrawNode.hpp"

#include "VulkanGraphicsEngineUtils.hpp"

namespace NVulkanEngine
{
	/* Implemented by derived class */
	void CDrawNode::Init(CGraphicsContext* context, SGraphicsManagers* managers) { }
	void CDrawNode::Draw(CGraphicsContext* context, SGraphicsManagers* managers, VkCommandBuffer commandBuffer) { }
	void CDrawNode::Cleanup(CGraphicsContext* context) { }

	void CDrawNode::GenerateMipmaps(CGraphicsContext* context, VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels)
	{
		// Check if image format supports linear blitting
		VkFormatProperties formatProperties;
		vkGetPhysicalDeviceFormatProperties(context->GetPhysicalDevice(), imageFormat, &formatProperties);

		if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT))
		{
			throw std::runtime_error("texture image format does not support linear blitting!");
		}

		VkCommandBuffer commandBuffer = BeginSingleTimeCommands(context);

		VkImageMemoryBarrier barrier{};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.image = image;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = 1;
		barrier.subresourceRange.levelCount = 1;

		int32_t mipWidth = texWidth;
		int32_t mipHeight = texHeight;

		for (uint32_t i = 1; i < mipLevels; i++)
		{
			barrier.subresourceRange.baseMipLevel = i - 1;
			barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

			vkCmdPipelineBarrier(
				commandBuffer,
				VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
				0, nullptr,
				0, nullptr,
				1, &barrier);

			VkImageBlit blit{};
			blit.srcOffsets[0] = { 0, 0, 0 };
			blit.srcOffsets[1] = { mipWidth, mipHeight, 1 };
			blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			blit.srcSubresource.mipLevel = i - 1;
			blit.srcSubresource.baseArrayLayer = 0;
			blit.srcSubresource.layerCount = 1;
			blit.dstOffsets[0] = { 0, 0, 0 };
			blit.dstOffsets[1] = { mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 };
			blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			blit.dstSubresource.mipLevel = i;
			blit.dstSubresource.baseArrayLayer = 0;
			blit.dstSubresource.layerCount = 1;

			vkCmdBlitImage(
				commandBuffer,
				image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				1, &blit,
				VK_FILTER_LINEAR);

			barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
			barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			// Transition to read optimal before submitting
			vkCmdPipelineBarrier(commandBuffer,
				VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
				0, nullptr,
				0, nullptr,
				1, &barrier);

			if (mipWidth > 1)  mipWidth /= 2;
			if (mipHeight > 1) mipHeight /= 2;
		}

		barrier.subresourceRange.baseMipLevel = mipLevels - 1;
		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		// Transition to read optimal before submitting
		vkCmdPipelineBarrier(commandBuffer,
			VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
			0, nullptr,
			0, nullptr,
			1, &barrier);

		EndSingleTimeCommands(context, commandBuffer);
	}

	void CDrawNode::BeginRendering(
		const std::string              markerName,
		CGraphicsContext*              context,
		VkCommandBuffer                commandBuffer,
		std::vector<SRenderAttachment> attachmentInfos)
	{
		std::vector<VkRenderingAttachmentInfo> colorAttachmentInfos = {};
		VkRenderingAttachmentInfo depthAttachmentInfo = {};
		bool hasDepthAttachment = false;
		for (uint32_t i = 0; i < attachmentInfos.size(); i++)
		{
			if (attachmentInfos[i].m_ImageUsage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)
			{
				colorAttachmentInfos.push_back(attachmentInfos[i].m_RenderAttachmentInfo);
			}
			if (attachmentInfos[i].m_ImageUsage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)
			{
				depthAttachmentInfo = attachmentInfos[i].m_RenderAttachmentInfo;
				hasDepthAttachment = true;
			}
		}

		VkRenderingInfo renderInfo{};
		renderInfo.sType                = VK_STRUCTURE_TYPE_RENDERING_INFO;
		renderInfo.renderArea.extent    = context->GetRenderResolution();
		renderInfo.renderArea.offset    = { 0, 0 };
		renderInfo.layerCount           = 1;
		renderInfo.colorAttachmentCount = static_cast<uint32_t>(colorAttachmentInfos.size());
		renderInfo.pColorAttachments    = colorAttachmentInfos.data();
		renderInfo.pDepthAttachment     = hasDepthAttachment ? &depthAttachmentInfo : nullptr;

		const float drawNodeMarkerColor[4] = { 0.4f, 0.6f, 0.3f, 1.0f };
		BeginMarker(context->GetVulkanInstance(), commandBuffer, markerName, drawNodeMarkerColor);
		vkCmdBeginRendering(commandBuffer, &renderInfo);


		VkViewport viewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = (float)context->GetRenderResolution().width;
		viewport.height = (float)context->GetRenderResolution().height;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

		VkRect2D scissor{};
		scissor.offset = { 0, 0 };
		scissor.extent = context->GetRenderResolution();

		vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
	}

	void CDrawNode::EndRendering(CGraphicsContext* context, VkCommandBuffer commandBuffer)
	{
		vkCmdEndRendering(commandBuffer);
		EndMarker(context->GetVulkanInstance(), commandBuffer);
	}
};