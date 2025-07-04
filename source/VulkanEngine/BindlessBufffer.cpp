#include "BindlessBuffer.hpp"
#include "DrawNodes/Utils/BindingTable.hpp"
#include <GraphicsContext.hpp>

#include <array>

// Support uniforms and textures currently
enum class EBindlessBufferType : uint32_t
{
	UNIFORMS = 0,
	TEXTURES = 1,
	COUNT    = 2
};

constexpr uint32_t g_MaxBindlessResources = 16536;

namespace NVulkanEngine
{
	// From https://dev.to/gasim/implementing-bindless-design-in-vulkan-34no
	uint32_t PadSizeToMinAlignment(uint32_t originalSize, uint32_t minAlignment) 
	{
		return (originalSize + minAlignment - 1) & ~(minAlignment - 1);
	}

	void CBindlessBuffer::AllocateDescriptorPool(CGraphicsContext* context)
	{
		std::array<VkDescriptorPoolSize, 1> poolSizeBindless[] =
		{
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, g_MaxBindlessResources },
			{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, g_MaxBindlessResources }
		};

		VkDescriptorPoolCreateInfo poolInfo{};
		poolInfo.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.flags         = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT;
		poolInfo.poolSizeCount = (uint32_t)poolSizeBindless->size();
		poolInfo.pPoolSizes    = poolSizeBindless->data();
		poolInfo.maxSets       = g_MaxBindlessResources * (uint32_t)poolSizeBindless->size();

		VkResult result = vkCreateDescriptorPool(context->GetLogicalDevice(), &poolInfo, nullptr, &m_DescriptorPool);

		if (result != VK_SUCCESS)
		{
			throw std::runtime_error("failed to allocate descriptor sets!");
		}
	}

	void CBindlessBuffer::AllocateDescriptorSetLayout(CGraphicsContext* context)
	{
		// Make sure order is the same as the enum!
		std::array<VkDescriptorType, (uint32_t)EBindlessBufferType::COUNT> descriptorBindingTypes =
		{
			VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER
		};

		std::array<VkDescriptorBindingFlags, (uint32_t) EBindlessBufferType::COUNT> descriptorBindingFlags
		{
			VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT_EXT | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT,
			VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT_EXT | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT
		};

		// Create layout for descriptor set
		std::array<VkDescriptorSetLayoutBinding, (uint32_t)EBindlessBufferType::COUNT> descriptorBindings{};
		
		for (uint32_t i = 0; i < (uint32_t) EBindlessBufferType::COUNT; i++)
		{
			descriptorBindings[i].binding = i;
			descriptorBindings[i].descriptorType = descriptorBindingTypes[i]; // uniforms/textures in this descriptor
			descriptorBindings[i].descriptorCount = g_MaxBindlessResources; // Up to 1000 uniforms/textures allowed
			descriptorBindings[i].stageFlags = VK_SHADER_STAGE_ALL;
		}

		VkDescriptorSetLayoutCreateInfo descriptorLayoutCreateInfo{};
		descriptorLayoutCreateInfo.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		descriptorLayoutCreateInfo.bindingCount = (uint32_t) EBindlessBufferType::COUNT;
		descriptorLayoutCreateInfo.pBindings    = descriptorBindings.data();
		descriptorLayoutCreateInfo.flags        = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;
		
		VkResult result = vkCreateDescriptorSetLayout(context->GetLogicalDevice(), &descriptorLayoutCreateInfo, nullptr, &m_DescriptorSetLayout);

		if (result != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create descriptor set layout!");
		}
	}

	void CBindlessBuffer::AllocateDescriptorSets(CGraphicsContext* context)
	{
		std::vector<VkDescriptorSetLayout> descritporSetLayouts(g_MaxFramesInFlight, m_DescriptorSetLayout);

		VkDescriptorSetAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = m_DescriptorPool;
		allocInfo.pSetLayouts = descritporSetLayouts.data();

		allocInfo.descriptorSetCount = g_MaxFramesInFlight;

		m_DescriptorSets.resize(g_MaxFramesInFlight);

		VkResult result = vkAllocateDescriptorSets(context->GetLogicalDevice(), &allocInfo, m_DescriptorSets.data());

		if (result != VK_SUCCESS)
		{
			throw std::runtime_error("failed to allocate descriptor sets!");
		}
	}

	void CBindlessBuffer::AddSampledImageBinding(VkDevice device, uint32_t bindingSlot, VkShaderStageFlagBits shaderStage, VkImageView imageView, VkFormat format, VkSampler sampler)
	{
		VkDescriptorSetLayoutBinding descriptorLayoutBinding = CreateDescriptorSetLayoutBinding(bindingSlot, shaderStage, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
		m_DescriptorSetLayoutBindings.push_back(descriptorLayoutBinding);

		const bool isDepthFormat = format >= VK_FORMAT_D16_UNORM && format <= VK_FORMAT_D32_SFLOAT_S8_UINT;

		SDescriptorInfo writeDescriptor{};
		writeDescriptor.m_BufferInfo = { VK_NULL_HANDLE, 0, VK_WHOLE_SIZE };
		writeDescriptor.m_ImageInfo = CreateDescriptorImageInfo(imageView, sampler, isDepthFormat ? VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		
		m_DescriptorInfos.push_back(writeDescriptor);
		
		VkWriteDescriptorSet write{};
		write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		write.dstBinding = (uint32_t)EBindlessBufferType::TEXTURES;
		write.dstSet = m_DescriptorSets[0];

		// Write just the one texture that is being added
		write.descriptorCount = 1;

		// The array element that we are going to write to
		// is the index, which we refer to as our handles
		write.dstArrayElement = bindingSlot;
		write.pImageInfo = &writeDescriptor.m_ImageInfo;

		vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);
		
		m_NumImageDescriptors++;
	}

	void CBindlessBuffer::CreateBindings(CGraphicsContext* context)
	{
		AllocateDescriptorPool(context);
		AllocateDescriptorSetLayout(context);
		AllocateDescriptorSets(context);


	}

	void CBindlessBuffer::BindTable(CGraphicsContext* context, VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout)
	{
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &m_DescriptorSets[context->GetFrameIndex()], 0, nullptr);
	}

	void CBindlessBuffer::Cleanup(CGraphicsContext* context)
	{
		vkDestroyDescriptorPool(context->GetLogicalDevice(), m_DescriptorPool, nullptr);
		vkDestroyDescriptorSetLayout(context->GetLogicalDevice(), m_DescriptorSetLayout, nullptr);

		m_DescriptorSets.clear();
		m_DescriptorInfos.clear();
		m_DescriptorSetLayoutBindings.clear();
		m_VertexInputAttributes.clear();
	}
};