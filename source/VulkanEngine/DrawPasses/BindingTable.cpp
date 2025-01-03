#include "BindingTable.hpp"
#include <GraphicsContext.hpp>

#include <array>

namespace NVulkanEngine
{



	void CBindingTable::SetVertexDescription(VkVertexInputBindingDescription vertexBindingDescription)
	{

	}

	void CBindingTable::AddVertexShaderAttribute(uint32_t locationSlot, VkFormat format, uint32_t offset)
	{
		VkVertexInputAttributeDescription attributeDescription{};

		attributeDescription.binding = 0;
		attributeDescription.location = locationSlot;
		attributeDescription.format = format;
		attributeDescription.offset = offset;

		m_VertexInputAttributes.push_back(attributeDescription);
	}

	VkDescriptorSetLayoutBinding CreateDescriptorSetLayoutBinding(uint32_t bindingSlot, VkShaderStageFlagBits shaderStage, VkDescriptorType descriptorType)
	{
		VkDescriptorSetLayoutBinding shaderDescriptorBinding{};

		shaderDescriptorBinding.binding            = bindingSlot;
		shaderDescriptorBinding.descriptorType     = descriptorType;
		shaderDescriptorBinding.descriptorCount    = 1;
		shaderDescriptorBinding.stageFlags         = shaderStage;
		shaderDescriptorBinding.pImmutableSamplers = nullptr;

		return shaderDescriptorBinding;
	}

	VkDescriptorBufferInfo CreateDescriptorBufferInfo(VkBuffer buffer, uint32_t range)
	{
		VkDescriptorBufferInfo bufferInfo{};
		bufferInfo.buffer = buffer;
		bufferInfo.offset = 0;
		bufferInfo.range  = range;

		return bufferInfo;
	}

	VkDescriptorImageInfo CreateDescriptorImageInfo(VkImageView imageView, VkSampler sampler, VkImageLayout imageLayout)
	{
		VkDescriptorImageInfo imageInfo{};
		imageInfo.imageLayout = imageLayout; // Make sure to transition the resource into this state before you use it
		imageInfo.imageView   = imageView;
		imageInfo.sampler     = sampler;

		return imageInfo;
	}

	void CBindingTable::AllocateDescriptorPool(CGraphicsContext* context)
	{
		uint32_t numBufferDescriptors = m_NumBufferDescriptors * g_MaxFramesInFlight;
		uint32_t numImageDescriptors  = m_NumImageDescriptors  * g_MaxFramesInFlight;
		uint32_t numDescriptorSets    = (uint32_t) m_DescriptorInfos.size() * g_MaxFramesInFlight;

		std::array<VkDescriptorPoolSize, 2> poolSizes{};
		poolSizes[0].type            = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		poolSizes[0].descriptorCount = numBufferDescriptors;
		poolSizes[1].type            = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		poolSizes[1].descriptorCount = numImageDescriptors;

		VkDescriptorPoolCreateInfo poolInfo{};
		poolInfo.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.poolSizeCount = m_NumImageDescriptors > 0 ? static_cast<uint32_t>(poolSizes.size()) : 1;
		poolInfo.pPoolSizes    = poolSizes.data();
		poolInfo.maxSets       = numDescriptorSets;

		VkResult result = vkCreateDescriptorPool(context->GetLogicalDevice(), &poolInfo, nullptr, &m_DescriptorPool);

		if (result != VK_SUCCESS)
		{
			throw std::runtime_error("failed to allocate descriptor sets!");
		}
	}


	void CBindingTable::AllocateDescriptorSetLayout(CGraphicsContext* context)
	{
		VkDescriptorSetLayoutCreateInfo layoutInfo{};
		layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutInfo.bindingCount = static_cast<uint32_t>(m_DescriptorSetLayoutBindings.size());
		layoutInfo.pBindings = m_DescriptorSetLayoutBindings.data();

		VkResult result = vkCreateDescriptorSetLayout(context->GetLogicalDevice(), &layoutInfo, nullptr, &m_DescriptorSetLayout);

		if (result != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create descriptor set layout!");
		}
	}

	void CBindingTable::AllocateDescriptorSets(CGraphicsContext* context)
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


	void CBindingTable::AddUniformBufferBinding(uint32_t bindingSlot, VkShaderStageFlagBits shaderStage, VkBuffer buffer, uint32_t bufferSize)
	{
		VkDescriptorSetLayoutBinding descriptorLayoutBinding = CreateDescriptorSetLayoutBinding(bindingSlot, shaderStage, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
		m_DescriptorSetLayoutBindings.push_back(descriptorLayoutBinding);
		
		SDescriptorInfo writeDescriptor{};
		writeDescriptor.m_BufferInfo = CreateDescriptorBufferInfo(buffer, bufferSize);
		writeDescriptor.m_ImageInfo  = {VK_NULL_HANDLE, VK_NULL_HANDLE, VK_IMAGE_LAYOUT_UNDEFINED };

		m_DescriptorInfos.push_back(writeDescriptor);
		m_NumBufferDescriptors++;
	}

	void CBindingTable::AddSampledImageBinding(uint32_t bindingSlot, VkShaderStageFlagBits shaderStage, VkImageView imageView, VkFormat format, VkSampler sampler)
	{
		VkDescriptorSetLayoutBinding descriptorLayoutBinding = CreateDescriptorSetLayoutBinding(bindingSlot, shaderStage, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
		m_DescriptorSetLayoutBindings.push_back(descriptorLayoutBinding);

		const bool isDepthFormat = format >= VK_FORMAT_D16_UNORM && format <= VK_FORMAT_D32_SFLOAT_S8_UINT;

		SDescriptorInfo writeDescriptor{};
		writeDescriptor.m_BufferInfo = { VK_NULL_HANDLE, 0, VK_WHOLE_SIZE };
		writeDescriptor.m_ImageInfo = CreateDescriptorImageInfo(imageView, sampler, isDepthFormat ? VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		
		m_DescriptorInfos.push_back(writeDescriptor);
		m_NumImageDescriptors++;
	}

	void CBindingTable::CreateBindings(CGraphicsContext* context)
	{
		AllocateDescriptorPool(context);
		AllocateDescriptorSetLayout(context);
		AllocateDescriptorSets(context);

		// Transfer our descriptor infos to a write descriptor
		for (uint32_t i = 0; i < m_DescriptorSets.size(); i++)
		{
			std::vector<VkWriteDescriptorSet> writeDescriptors = {};
			writeDescriptors.resize(m_DescriptorInfos.size());

			VkDescriptorSet descriptorSet = m_DescriptorSets[i];

			// Now add the descriptors
			for (uint32_t j = 0; j < writeDescriptors.size(); j++)
			{
				VkDescriptorType descriptorType = m_DescriptorSetLayoutBindings[j].descriptorType;

				writeDescriptors[j].descriptorType = descriptorType;
				writeDescriptors[j].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				writeDescriptors[j].dstSet = descriptorSet;
				writeDescriptors[j].dstBinding = j;
				writeDescriptors[j].descriptorCount = 1;
				writeDescriptors[j].descriptorType = descriptorType;
				writeDescriptors[j].dstArrayElement = 0;

				if (descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER)
				{
					writeDescriptors[j].pBufferInfo = &m_DescriptorInfos[j].m_BufferInfo;
					writeDescriptors[j].pImageInfo = VK_NULL_HANDLE;
				}
				else if (descriptorType == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
				{
					writeDescriptors[j].pBufferInfo = VK_NULL_HANDLE;
					writeDescriptors[j].pImageInfo = &m_DescriptorInfos[j].m_ImageInfo;
				}
			}

			vkUpdateDescriptorSets(context->GetLogicalDevice(), static_cast<uint32_t>(writeDescriptors.size()), writeDescriptors.data(), 0, nullptr);
		}
	}

	void CBindingTable::BindTable(CGraphicsContext* context, VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout)
	{
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &m_DescriptorSets[context->GetFrameIndex()], 0, nullptr);
	}

	void CBindingTable::Cleanup(CGraphicsContext* context)
	{
		vkDestroyDescriptorPool(context->GetLogicalDevice(), m_DescriptorPool, nullptr);
		vkDestroyDescriptorSetLayout(context->GetLogicalDevice(), m_DescriptorSetLayout, nullptr);

		m_DescriptorInfos.clear();
		m_DescriptorSetLayoutBindings.clear();
		m_VertexInputAttributes.clear();
	}
};