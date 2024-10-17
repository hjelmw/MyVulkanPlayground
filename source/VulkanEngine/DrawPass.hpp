#pragma once

#include <vulkan/vulkan.hpp>

#include "GraphicsContext.hpp"

#include "Swapchain.hpp"

#include <glm/glm.hpp>
#include <array>

enum class ERenderAttachments : uint32_t
{
	Positions      = 0,
	Normals        = 1,
	Albedo         = 2,
	Depth          = 3,
	ShadowMap      = 4,
	SceneColor     = 5,
	Count          = 6
};


namespace NVulkanEngine
{
	class CDrawPass
	{
	public:
		virtual void InitPass(CGraphicsContext* context);
		virtual void Draw(CGraphicsContext* context, VkCommandBuffer commandBuffer);
		virtual void CleanupPass(CGraphicsContext* context);

	protected:
		enum EAttachmentType
		{
			EColorAttachment = 0,
			EDepthAttachment = 1,
			EAttachmentCount = 2 
		};
		enum EAttachmentUsage
		{
			EReadAttachment      = 0,
			EWriteAttachment     = 1,
			EReadWriteAttachment = 2,
			EAttachmentUsageCount = 3
		};

		// A draw pass attachment
		struct SImageAttachment
		{
			EAttachmentType           m_Type;
			EAttachmentUsage          m_Usage;
			VkImage                   m_Image;
			VkDeviceMemory            m_Memory;
			VkImageView               m_ImageView;
			VkFormat                  m_Format;
			VkRenderingAttachmentInfo m_RenderAttachmentInfo;
		};

		// Creates a sampler with the provided parameters
		VkSampler CreateSampler(
			CGraphicsContext* context,
			VkSamplerAddressMode samplerModeU,
			VkSamplerAddressMode samplerModeV,
			VkSamplerAddressMode samplerModeW,
			VkSamplerMipmapMode  samplerMipmapMode,
			VkFilter             minFilter,
			VkFilter             magFilter,
			float                lodbias,
			float                minLod,
			float                maxLod);

		// Creates a attachment with associated vulkan image & view
		SImageAttachment CreateAttachment(
			CGraphicsContext*       context,
			VkFormat                format,
			VkImageUsageFlags       usage,
			VkImageLayout           imageLayout,
			uint32_t                width,
			uint32_t                height);

		// Creates the descriptor pool from which descriptor sets can be allocated from
		void AllocateDescriptorPool(
			CGraphicsContext* context,
			uint32_t          numDescriptorSets,
			uint32_t          descriptorImageCount,
			uint32_t          descriptorBufferCount);

		// Update descriptor set layout with a write descriptor set (vector of image views)
		void UpdateDescriptorSets(
			CGraphicsContext*                 context, 
			std::vector<VkDescriptorSet>      descriptorSets,
			std::vector<VkWriteDescriptorSet> writeDescriptorSets);

		// Build vulkan descriptor image & buffer structs
		VkDescriptorImageInfo  CreateDescriptorImageInfo(VkSampler sampler, VkImageView imageView, VkImageLayout imageLayout);
		VkDescriptorBufferInfo CreateDescriptorBufferInfo(VkBuffer uniformBuffer, uint32_t range);

		// Create write descriptors at the specified binding and slot
		VkWriteDescriptorSet CreateWriteDescriptorImage(
			CGraphicsContext*      context, 
			VkDescriptorSet*       descriptorSets, 
			VkDescriptorType       descriptorType, 
			uint32_t               descriptorSlot, 
			VkDescriptorImageInfo* descriptorImageInfo);

		// Create write descriptor for buffer at the specified binding and slot
		VkWriteDescriptorSet CreateWriteDescriptorBuffer(
			CGraphicsContext*       context, 
			VkDescriptorSet*        descriptorSets, 
			VkDescriptorType        descriptorType, 
			uint32_t                descriptorSlot, 
			VkDescriptorBufferInfo* descriptorBufferInfo);

		
		void GenerateMipmaps(
			CGraphicsContext* context, 
			VkImage           image, 
			VkFormat          imageFormat, 
			int32_t           texWidth, 
			int32_t           texHeight, 
			uint32_t          mipLevels);

		SImageAttachment GetSwapchainAttachment(CGraphicsContext* context);

		// Begin rendering with attachments
		void BeginRendering(
			CGraphicsContext*             context,
			VkCommandBuffer               commandBuffer,
			std::vector<SImageAttachment> attachmentInfos);

		void EndRendering(VkCommandBuffer commandBuffer);

		void PushConstants(VkCommandBuffer commandBuffer, VkShaderStageFlags shaderStage, void* data, size_t constantsSize);

		// Copies buffer into image
		void CopyBufferToImage(
			CGraphicsContext* context, 
			VkBuffer buffer, 
			VkImage image, 
			uint32_t width, 
			uint32_t height);

		std::vector<SImageAttachment> m_Attachments = {};

		VkDescriptorSetLayout        m_DescriptorSetLayout   = VK_NULL_HANDLE;
		VkPipelineLayout             m_PipelineLayout        = VK_NULL_HANDLE;
		VkDescriptorPool             m_DescriptorPool        = VK_NULL_HANDLE;
	};
};
