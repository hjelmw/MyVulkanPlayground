#pragma once 

#include <vulkan/vulkan.h>

// standard library
#include <cstdlib>
#include <vector>
#include <set>
#include <iostream>
#include <fstream>
#include <array>
#include <algorithm>
#include <optional>
#include <unordered_map>

#include "GraphicsContext.hpp"

namespace NVulkanEngine
{
	static const std::vector<const char*> g_ValidationLayers = { "VK_LAYER_KHRONOS_validation" };
	static const std::vector<const char*> g_DeviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME, VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME };

#if defined(_DEBUG)
	static const bool g_EnableValidationLayers = true;
#else
	static const bool g_EnableValidationLayers = false;
#endif

	struct SRenderAttachment
	{
		char                      m_DebugName[64]        = "No Debug Name";
		VkFormat                  m_Format               = VK_FORMAT_UNDEFINED;
		VkImage                   m_Image                = VK_NULL_HANDLE;
		VkImageView               m_ImageView            = VK_NULL_HANDLE;
		VkImageUsageFlags         m_ImageUsage           = VK_IMAGE_USAGE_FLAG_BITS_MAX_ENUM;
		VkImageLayout             m_CurrentImageLayout   = VK_IMAGE_LAYOUT_UNDEFINED;
		VkDeviceMemory            m_Memory               = VK_NULL_HANDLE;
		VkDescriptorSet           m_ImguiDescriptor      = VK_NULL_HANDLE;
		VkRenderingAttachmentInfo m_RenderAttachmentInfo = {};
	};

	static std::vector<const char*> GetRequiredInstanceExtensions()
	{
		uint32_t requiredGLFWExtensionCount;
		const char** requiredGLFWExtensions = glfwGetRequiredInstanceExtensions(&requiredGLFWExtensionCount);

		std::vector<const char*> requiredExtensions(requiredGLFWExtensions, requiredGLFWExtensions + requiredGLFWExtensionCount);
		if (g_EnableValidationLayers)
		{
			requiredExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		}

		return requiredExtensions;
	}

	static std::vector<char> ReadFile(const std::string& filename)
	{
		std::ifstream file(filename, std::ios::ate | std::ios::binary);

		if (!file.is_open())
		{
			throw std::runtime_error(std::string("Failed to open shader file: " + filename).c_str());
		}

		size_t fileSize = (size_t)file.tellg();
		std::vector<char> buffer(fileSize);

		file.seekg(0);
		file.read(buffer.data(), fileSize);
		file.close();

#if defined(_DEBUG)
		std::cout << "Shader module [name: " << filename << ", size: " << fileSize << " bytes, ";
#endif

		return buffer;
	};

	static VkShaderModule CreateShaderModule(VkDevice device, const std::string& filename)
	{
		auto shaderCode = ReadFile(filename);

		VkShaderModuleCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createInfo.codeSize = shaderCode.size();
		createInfo.pCode = reinterpret_cast<const uint32_t*>(shaderCode.data());

		VkShaderModule shaderModule;
		if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
		{
			throw std::runtime_error(std::string("Failed to created shader module for '" + filename + "'").c_str());
		}

#if defined(_DEBUG)
		std::cout << "pointer: 0x" << reinterpret_cast<void*>(shaderCode.data()) << "] " << "loaded" << std::endl;
#endif

		return shaderModule;
	};

	struct SVulkanQueueFamilyIndices
	{
		std::optional<uint32_t> m_GraphicsFamily;
		std::optional<uint32_t> m_PresentFamily;

		bool IsComplete()
		{
			return m_GraphicsFamily.has_value() && m_PresentFamily.has_value();
		};
	};

	static SVulkanQueueFamilyIndices FindVulkanQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface)
	{
		SVulkanQueueFamilyIndices indices;

		uint32_t queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

		std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

		int i = 0;
		for (const auto& queueFamily : queueFamilies)
		{
			VkBool32 presentSupport = false;
			vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);

			if (presentSupport)
			{
				indices.m_PresentFamily = i;
			}
			if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
			{
				indices.m_GraphicsFamily = i;
			}

			if (indices.IsComplete())
			{
				break;
			}

			i++;
		}

		return indices;
	}

	/*
	*
	* Validation layers
	*
	*/

	static bool CheckValidationLayerSupport()
	{
		uint32_t layerCount;
		vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

		std::vector<VkLayerProperties> availableLayers(layerCount);
		vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

		// Find validation layer support
		for (const char* layerName : g_ValidationLayers)
		{
			bool layerFound = false;
			for (const auto& layerProperties : availableLayers)
			{
				if (strcmp(layerName, layerProperties.layerName) == 0)
				{
					std::cout << "Validation layers enabled: " << layerName << " Enabled\n" << std::endl;
					layerFound = true;
					break;
				}
			}

			if (!layerFound)
			{
				return false;
			}
		}

		return true;
	}

	/*
	*
	* Image format
	*
	*/

	static VkFormat FindSupportedFormat(VkPhysicalDevice physicalDevice, const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features)
	{
		for (VkFormat format : candidates)
		{
			VkFormatProperties props;
			vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &props);

			if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features)
			{
				return format;
			}
			else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features)
			{
				return format;
			}
		}

		throw std::runtime_error("failed to find supported format!");
	}

	static VkFormat FindDepthFormat(VkPhysicalDevice physicalDevice)
	{
		return FindSupportedFormat(
			physicalDevice,
			{ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
			VK_IMAGE_TILING_OPTIMAL,
			VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
	}

	static int FindMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties)
	{
		VkPhysicalDeviceMemoryProperties memoryProperties;
		vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);

		for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++)
		{
			// Is memory type supported
			if (typeFilter & (1 << i) && (memoryProperties.memoryTypes[i].propertyFlags & properties) == properties)
			{
				return i;
			}
		}

		return -1;
	}

	static VkSampleCountFlagBits GetMaxUsableSampleCount(VkPhysicalDevice physicalDevice)
	{
		VkPhysicalDeviceProperties physicalDeviceProperties;
		vkGetPhysicalDeviceProperties(physicalDevice, &physicalDeviceProperties);

		VkSampleCountFlags counts = physicalDeviceProperties.limits.framebufferColorSampleCounts & physicalDeviceProperties.limits.framebufferDepthSampleCounts;
		if (counts & VK_SAMPLE_COUNT_64_BIT) { return VK_SAMPLE_COUNT_64_BIT; }
		if (counts & VK_SAMPLE_COUNT_32_BIT) { return VK_SAMPLE_COUNT_32_BIT; }
		if (counts & VK_SAMPLE_COUNT_16_BIT) { return VK_SAMPLE_COUNT_16_BIT; }
		if (counts & VK_SAMPLE_COUNT_8_BIT) { return VK_SAMPLE_COUNT_8_BIT; }
		if (counts & VK_SAMPLE_COUNT_4_BIT) { return VK_SAMPLE_COUNT_4_BIT; }
		if (counts & VK_SAMPLE_COUNT_2_BIT) { return VK_SAMPLE_COUNT_2_BIT; }

		return VK_SAMPLE_COUNT_1_BIT;
	}

	static void AllocateDescriptorPool(
		CGraphicsContext* context,
		VkDescriptorPool& descriptorPool,
		uint32_t          numDescriptorSets,
		uint32_t          descriptorImageCount,
		uint32_t          descriptorBufferCount)
	{
		std::array<VkDescriptorPoolSize, 2> poolSizes{};
		poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		poolSizes[0].descriptorCount = descriptorBufferCount;

		poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		poolSizes[1].descriptorCount = descriptorImageCount;

		VkDescriptorPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.poolSizeCount = descriptorImageCount > 0 ? static_cast<uint32_t>(poolSizes.size()) : 1;
		poolInfo.pPoolSizes = poolSizes.data();
		poolInfo.maxSets = numDescriptorSets;

		if (vkCreateDescriptorPool(context->GetLogicalDevice(), &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create descriptor pool!");
		}
	}

	static std::vector<VkDescriptorSet> AllocateDescriptorSets(
		CGraphicsContext*     context,
		VkDescriptorPool      descriptorPool,
		VkDescriptorSetLayout descriptorSetLayout,
		uint32_t              numDescriptorSets)
	{
		std::vector<VkDescriptorSetLayout> layouts(numDescriptorSets, descriptorSetLayout);

		VkDescriptorSetAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = descriptorPool;
		allocInfo.pSetLayouts = layouts.data();
		
		allocInfo.descriptorSetCount = numDescriptorSets;

		std::vector<VkDescriptorSet> descriptorSets(numDescriptorSets);

		VkResult result = vkAllocateDescriptorSets(context->GetLogicalDevice(), &allocInfo, descriptorSets.data());

		if (result != VK_SUCCESS)
		{
			throw std::runtime_error("failed to allocate descriptor sets!");
		}

		return descriptorSets;
	}

	static void UpdateDescriptorSets(
		CGraphicsContext* context,
		std::vector<VkDescriptorSet> descriptorSets,
		std::vector<VkWriteDescriptorSet> writeDescriptorSets)
	{
		// One set per frame in flight
		for (uint32_t i = 0; i < descriptorSets.size(); i++)
		{
			for (uint32_t j = 0; j < writeDescriptorSets.size(); j++)
			{
				writeDescriptorSets[j].dstSet = descriptorSets[i];
			}
			vkUpdateDescriptorSets(context->GetLogicalDevice(), static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, nullptr);
		}

	}

	static VkCommandBuffer BeginSingleTimeCommands(CGraphicsContext* context)
	{
		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandPool = context->GetCommandPool();
		allocInfo.commandBufferCount = 1;

		VkCommandBuffer commandBuffer;
		vkAllocateCommandBuffers(context->GetLogicalDevice(), &allocInfo, &commandBuffer);

		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		vkBeginCommandBuffer(commandBuffer, &beginInfo);

		return commandBuffer;
	}

	static void EndSingleTimeCommands(CGraphicsContext* context, VkCommandBuffer commandBuffer)
	{
		vkEndCommandBuffer(commandBuffer);

		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffer;

		vkQueueSubmit(context->GetGraphicsQueue(), 1, &submitInfo, VK_NULL_HANDLE);
		vkQueueWaitIdle(context->GetGraphicsQueue());
		vkFreeCommandBuffers(context->GetLogicalDevice(), context->GetCommandPool(), 1, &commandBuffer);
	}

	static VkBuffer CreateBuffer(
		CGraphicsContext*     context, 
		VkDeviceMemory&       bufferMemory, 
		VkDeviceSize          size, 
		VkBufferUsageFlags    usage, 
		VkMemoryPropertyFlags properties)
	{
		VkBufferCreateInfo bufferInfo{};

		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.usage = usage;
		bufferInfo.size = size;
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		VkBuffer buffer;

		if (vkCreateBuffer(context->GetLogicalDevice(), &bufferInfo, nullptr, &buffer) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create vertex buffer!");
		}

		VkMemoryRequirements memoryRequirements;
		vkGetBufferMemoryRequirements(context->GetLogicalDevice(), buffer, &memoryRequirements);

		VkMemoryAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memoryRequirements.size;
		allocInfo.memoryTypeIndex = FindMemoryType(context->GetPhysicalDevice(), memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

		if (vkAllocateMemory(context->GetLogicalDevice(), &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to allocate vertex buffer memory!");
		}

		vkBindBufferMemory(context->GetLogicalDevice(), buffer, bufferMemory, 0);

		return buffer;
	}

	static void CopyBuffer(CGraphicsContext* context, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
	{
		VkCommandBuffer commandBuffer = BeginSingleTimeCommands(context);

		VkBufferCopy copyRegion{};
		copyRegion.srcOffset = 0; // Optional
		copyRegion.dstOffset = 0; // Optional
		copyRegion.size = size;
		vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

		EndSingleTimeCommands(context, commandBuffer);
	}

	static VkImage CreateImage(
		CGraphicsContext* context,
		uint32_t              width,
		uint32_t              height,
		uint32_t              mipLevels,
		VkSampleCountFlagBits numSamples,
		VkFormat              format,
		VkImageTiling         tiling,
		VkImageUsageFlags     usage,
		VkMemoryPropertyFlags properties,
		VkDeviceMemory& imageMemory)
	{
		VkImage image;

		VkImageCreateInfo imageInfo{};
		imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageInfo.imageType = VK_IMAGE_TYPE_2D;
		imageInfo.extent.width = width;
		imageInfo.extent.height = height;
		imageInfo.extent.depth = 1;
		imageInfo.mipLevels = mipLevels;
		imageInfo.samples = numSamples;
		imageInfo.arrayLayers = 1;
		imageInfo.format = format;
		imageInfo.tiling = tiling;
		imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageInfo.usage = usage;
		imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		if (vkCreateImage(context->GetLogicalDevice(), &imageInfo, nullptr, &image) != VK_SUCCESS) {
			throw std::runtime_error("failed to create image!");
		}

		VkMemoryRequirements memRequirements;
		vkGetImageMemoryRequirements(context->GetLogicalDevice(), image, &memRequirements);

		VkMemoryAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memRequirements.size;
		allocInfo.memoryTypeIndex = FindMemoryType(context->GetPhysicalDevice(), memRequirements.memoryTypeBits, properties);

		if (vkAllocateMemory(context->GetLogicalDevice(), &allocInfo, nullptr, &imageMemory) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to allocate image memory!");
		}

		vkBindImageMemory(context->GetLogicalDevice(), image, imageMemory, 0);

		return image;
	}

	static void CopyBufferToImage(CGraphicsContext* context, VkBuffer buffer, VkImage image, uint32_t width, uint32_t height)
	{
		VkCommandBuffer commandBuffer = BeginSingleTimeCommands(context);

		VkBufferImageCopy region{};
		region.bufferOffset = 0;
		region.bufferRowLength = 0;
		region.bufferImageHeight = 0;
		region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		region.imageSubresource.mipLevel = 0;
		region.imageSubresource.baseArrayLayer = 0;
		region.imageSubresource.layerCount = 1;
		region.imageOffset = { 0, 0, 0 };
		region.imageExtent = { width, height, 1 };

		vkCmdCopyBufferToImage(
			commandBuffer,
			buffer,
			image,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1,
			&region
		);

		EndSingleTimeCommands(context, commandBuffer);
	}

	static VkImageView CreateImageView(CGraphicsContext* context, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels)
	{
		VkImageViewCreateInfo viewInfo{};
		viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewInfo.image = image;
		viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		viewInfo.format = format;
		viewInfo.subresourceRange.aspectMask = aspectFlags;
		viewInfo.subresourceRange.baseMipLevel = 0;
		viewInfo.subresourceRange.levelCount = mipLevels;
		viewInfo.subresourceRange.baseArrayLayer = 0;
		viewInfo.subresourceRange.layerCount = 1;

		VkImageView imageView;
		if (vkCreateImageView(context->GetLogicalDevice(), &viewInfo, nullptr, &imageView) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create renderAttachment image view!");
		}

		return imageView;
	}

	static void TransitionImageLayout(
		VkCommandBuffer    commandBuffer,
		SRenderAttachment& renderAttachment,
		VkImageLayout      newLayout,
		uint32_t           mipLevels)
	{
		VkImageMemoryBarrier barrier{};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.oldLayout = renderAttachment.m_CurrentImageLayout;
		barrier.newLayout = newLayout;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.image = renderAttachment.m_Image;
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		barrier.subresourceRange.baseMipLevel = 0;
		barrier.subresourceRange.levelCount = mipLevels;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = 1;

		VkPipelineStageFlags sourceStage;
		VkPipelineStageFlags destinationStage;

		bool depthLayout = renderAttachment.m_CurrentImageLayout == VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL || newLayout == VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL ||
			renderAttachment.m_CurrentImageLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL || newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		if (depthLayout)
		{
			barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

			bool hasStencilComponent = renderAttachment.m_Format == VK_FORMAT_D32_SFLOAT_S8_UINT 
									|| renderAttachment.m_Format == VK_FORMAT_D24_UNORM_S8_UINT;

			if (hasStencilComponent)
			{
				barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
			}
		}
		else
		{
			barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		}

		if (renderAttachment.m_CurrentImageLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_GENERAL)
		{
			barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;

			sourceStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
			destinationStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		}
		else if (renderAttachment.m_CurrentImageLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
		{
			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

			sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		}
		else if (renderAttachment.m_CurrentImageLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
		{
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		}
		else if (renderAttachment.m_CurrentImageLayout == VK_IMAGE_LAYOUT_UNDEFINED && (newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL || newLayout == VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL))
		{
			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		}
		else if (renderAttachment.m_CurrentImageLayout == VK_IMAGE_LAYOUT_UNDEFINED && (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL || newLayout == VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL))
		{
			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

			sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		}
		else if (renderAttachment.m_CurrentImageLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
		{
			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

			sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			destinationStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		}
		else if (renderAttachment.m_CurrentImageLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR)
		{
			barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			barrier.dstAccessMask = 0;

			sourceStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			destinationStage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
		}
		else if (renderAttachment.m_CurrentImageLayout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR && newLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
		{
			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

			sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			destinationStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		}
		else if (renderAttachment.m_CurrentImageLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR)
		{
			barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
			barrier.dstAccessMask = 0;

			sourceStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
			destinationStage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
		}
		else if (renderAttachment.m_CurrentImageLayout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
		{
			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;

			sourceStage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
			destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		}
		else if (renderAttachment.m_CurrentImageLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
		{
			barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;

			sourceStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			destinationStage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
		}
		else if (renderAttachment.m_CurrentImageLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
		{
			barrier.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
			barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

			sourceStage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
			destinationStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		}
		else if ((renderAttachment.m_CurrentImageLayout == VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL || renderAttachment.m_CurrentImageLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
				&& (newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL || newLayout == VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL))
		{
			barrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;

			sourceStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
			destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		}
		else if (renderAttachment.m_CurrentImageLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
		{
			barrier.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
			barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

			sourceStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
			destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		}
		else if (renderAttachment.m_CurrentImageLayout == VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
		{
			barrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;

			sourceStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
			destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		}
		else if ((renderAttachment.m_CurrentImageLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL || renderAttachment.m_CurrentImageLayout == VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL)
				&& (newLayout == VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL || renderAttachment.m_CurrentImageLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL))
		{
			barrier.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
			barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

			sourceStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
			destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		}
		else
		{
			std::cout << "Transition Image Layout failed due to unsupported layout transition!" << std::endl;
			throw std::invalid_argument("unsupported layout transition!");
		}

		renderAttachment.m_CurrentImageLayout = newLayout;
		renderAttachment.m_RenderAttachmentInfo.imageLayout = newLayout;

		vkCmdPipelineBarrier(
			commandBuffer,
			sourceStage,
			destinationStage,
			0,
			0, nullptr,
			0, nullptr,
			1, &barrier
		);
	}


	// Same as above but creates the renderattachment for you and also creates and consumes a new command buffer
	static void SingleTimeTransitionImageLayout(CGraphicsContext* context, SRenderAttachment& renderAttachment, VkImageLayout newLayout, uint32_t mipLevels)
	{
		VkCommandBuffer commandBuffer = BeginSingleTimeCommands(context);
		TransitionImageLayout(commandBuffer, renderAttachment, newLayout, mipLevels);
		EndSingleTimeCommands(context, commandBuffer);
	}

	// Sets up viewport and scissor rectangle
	static void SetViewportScissor(VkCommandBuffer commandBuffer, VkExtent2D viewportScissorDimensions)
	{
		VkViewport viewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = static_cast<float>(viewportScissorDimensions.width);
		viewport.height = static_cast<float>(viewportScissorDimensions.height);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

		VkRect2D scissor{};
		scissor.offset = { 0, 0 };
		scissor.extent.width = viewportScissorDimensions.width;
		scissor.extent.height = viewportScissorDimensions.height;
		vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
	}

	static SRenderAttachment CreateRenderAttachment
	(
		CGraphicsContext* context,
		VkFormat          format,
		VkImageUsageFlags usage,
		VkImageLayout     imageLayout,
		uint32_t          width,
		uint32_t          height
	)
	{
		SRenderAttachment renderAttachment{};
		renderAttachment.m_Format = format;
		renderAttachment.m_ImageUsage = usage;
		renderAttachment.m_Image = CreateImage(
			context,
			width,
			height,
			1,
			VK_SAMPLE_COUNT_1_BIT,
			format,
			VK_IMAGE_TILING_OPTIMAL,
			usage,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			renderAttachment.m_Memory);

		VkClearValue clearValue{};
		VkImageAspectFlags aspectFlags = VK_IMAGE_ASPECT_NONE_KHR;

		if (usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)
		{
			aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;
			clearValue.color = { 0.0f, 0.0f, 0.0f, 0.0f };
		}
		else if (usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)
		{
			aspectFlags = VK_IMAGE_ASPECT_DEPTH_BIT;
			clearValue.depthStencil = { 1.0f, 0 };
		}

		renderAttachment.m_ImageView = CreateImageView(
			context,
			renderAttachment.m_Image,
			format,
			aspectFlags,
			1);

		VkRenderingAttachmentInfo renderInfo{};
		renderInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
		renderInfo.imageView = renderAttachment.m_ImageView;
		renderInfo.imageLayout = imageLayout;
		renderInfo.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		renderInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		renderInfo.clearValue = clearValue;

		renderAttachment.m_RenderAttachmentInfo = renderInfo;

		SingleTimeTransitionImageLayout(context, renderAttachment, imageLayout, 1);

		renderAttachment.m_CurrentImageLayout = imageLayout;

		return renderAttachment;
	}

	static VkSampler CreateSampler(
		CGraphicsContext* context,
		VkSamplerAddressMode samplerModeU,
		VkSamplerAddressMode samplerModeV,
		VkSamplerAddressMode samplerModeW,
		VkSamplerMipmapMode  samplerMipmapMode,
		VkFilter             minFilter,
		VkFilter             magFilter,
		float                lodbias,
		float                minLod,
		float                maxLod
	)
	{
		VkSamplerCreateInfo samplerInfo{};
		samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerInfo.minFilter = minFilter;
		samplerInfo.magFilter = magFilter;
		samplerInfo.mipmapMode = samplerMipmapMode;
		samplerInfo.addressModeU = samplerModeU;
		samplerInfo.addressModeV = samplerModeV;
		samplerInfo.addressModeW = samplerModeW;
		samplerInfo.mipLodBias = lodbias;
		samplerInfo.maxAnisotropy = 1.0f;
		samplerInfo.minLod = minLod;
		samplerInfo.maxLod = maxLod;
		samplerInfo.unnormalizedCoordinates = VK_FALSE;

		samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;

		VkSampler sampler = VK_NULL_HANDLE;
		if (vkCreateSampler(context->GetLogicalDevice(), &samplerInfo, nullptr, &sampler) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create sampler for geometry render pass!");
		}

		return sampler;
	}
}
