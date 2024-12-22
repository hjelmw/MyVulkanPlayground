#include "Texture.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include "stbi_image.h"

#include "VulkanGraphicsEngineUtils.hpp"

namespace NVulkanEngine 
{

	void CTexture::CreateTextureImage(CGraphicsContext* context, stbi_uc* imagePixels, uint32_t texWidth, uint32_t texHeight, VkFormat format)
	{
		VkDeviceSize textureImageSize = texWidth * texHeight * STBI_rgb_alpha;

		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;

		/* CPU Side staging buffer */
		stagingBuffer = CreateBuffer(
			context,
			stagingBufferMemory,
			textureImageSize,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

		void* data;
		vkMapMemory(context->GetLogicalDevice(), stagingBufferMemory, 0, textureImageSize, 0, &data);
		memcpy(data, imagePixels, static_cast<size_t>(textureImageSize));
		vkUnmapMemory(context->GetLogicalDevice(), stagingBufferMemory);

		stbi_image_free(imagePixels);

		/* GPU side texture */
		m_TextureImage = CreateImage(
			context,
			texWidth,
			texHeight,
			m_MipLevels,
			VK_SAMPLE_COUNT_1_BIT,
			format,
			VK_IMAGE_TILING_OPTIMAL,
			VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			m_TextureImageMemory);

		m_TextureFormat = format;

		if (m_GenerateMipmaps)
		{
			m_MipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;
			GenerateMipmaps(context, m_TextureImage, format, texWidth, texHeight, m_MipLevels);
		}

		SRenderAttachment textureRenderAttachment{};
		textureRenderAttachment.m_Format = format;
		textureRenderAttachment.m_Image = m_TextureImage;
		textureRenderAttachment.m_ImageView = m_TextureImageView;
		textureRenderAttachment.m_ImageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		textureRenderAttachment.m_Memory = VK_NULL_HANDLE; // Not needed

		VkRenderingAttachmentInfo renderInfo{};
		renderInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
		renderInfo.imageView = VK_NULL_HANDLE; // Not created yet
		renderInfo.imageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		renderInfo.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
		renderInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

		std::vector<VkRenderingAttachmentInfo> renderAttachmentInfos = { renderInfo };
		textureRenderAttachment.m_RenderAttachmentInfo = renderInfo;

		SingleTimeTransitionImageLayout(context, textureRenderAttachment, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, m_MipLevels);

		CopyBufferToImage(context, stagingBuffer, m_TextureImage, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));

		SingleTimeTransitionImageLayout(context, textureRenderAttachment, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, m_MipLevels);

		vkDestroyBuffer(context->GetLogicalDevice(), stagingBuffer, nullptr);
		vkFreeMemory(context->GetLogicalDevice(), stagingBufferMemory, nullptr);
	}

	void CTexture::CreateTextureImageView(CGraphicsContext* context)
	{
		m_TextureImageView = CreateImageView(context, m_TextureImage, m_TextureFormat, VK_IMAGE_ASPECT_COLOR_BIT, m_MipLevels);
	}

	void CTexture::CreateTexture(CGraphicsContext* context, std::string textureFilepath, VkFormat format)
	{
		int texWidth, texHeight, texChannels;
		stbi_uc* pixels = stbi_load(textureFilepath.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

		if (!pixels)
		{
			throw std::runtime_error("failed to load texture image!");
		}

		CreateTextureImage(context, pixels, texWidth, texHeight, format);
		CreateTextureImageView(context);
	}

	void CTexture::GenerateMipmaps(CGraphicsContext* context, VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels)
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

	void CTexture::DestroyTexture(CGraphicsContext* context)
	{
		vkDestroyImageView(context->GetLogicalDevice(), m_TextureImageView, nullptr);

		vkDestroyImage(context->GetLogicalDevice(), m_TextureImage, nullptr);
		vkFreeMemory(context->GetLogicalDevice(),   m_TextureImageMemory, nullptr);

		m_MipLevels = 1;
		m_GenerateMipmaps = false;
	}

}