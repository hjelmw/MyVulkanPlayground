#pragma once

#include <vulkan/vulkan.h>
#include <VulkanGraphicsEngineUtils.hpp>

#include <vector>

/*
	Simplifies shader resource binding. Used by the draw nodes
*/

namespace NVulkanEngine
{
	class CBindingTable
	{
	public:
		CBindingTable() = default;
		~CBindingTable() = default;

		void SetVertexDescription(VkVertexInputBindingDescription vertexBindingDescription);
		void AddVertexShaderAttribute(uint32_t locationSlot, VkFormat format, uint32_t offset);
		void AddUniformBufferBinding(uint32_t bindingSlot, VkShaderStageFlagBits shaderStage, VkBuffer buffer, uint32_t bufferSize);
		void AddSampledImageBinding(uint32_t bindingSlot, VkShaderStageFlagBits shaderStage, VkImageView imageView, VkFormat format, VkSampler sampler);
		void CreateBindings(CGraphicsContext* context);

		VkDescriptorSetLayout GetDescriptorSetLayout() { return m_DescriptorSetLayout; };

		void BindTable(CGraphicsContext* context, VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout);

		void Cleanup(CGraphicsContext* context);

	private:
		void AllocateDescriptorPool(CGraphicsContext* context);
		void AllocateDescriptorSetLayout(CGraphicsContext* context);
		void AllocateDescriptorSets(CGraphicsContext* context);

		std::vector<VkVertexInputAttributeDescription> m_VertexInputAttributes = { };

		struct SDescriptorInfo
		{
			VkDescriptorBufferInfo m_BufferInfo = { };
			VkDescriptorImageInfo  m_ImageInfo = { };
		};

		// Descriptors 
		std::vector<SDescriptorInfo>              m_DescriptorInfos = { }; // Holder for the current descriptors. Consumed in CreateBindings()
		std::vector<VkDescriptorSetLayoutBinding> m_DescriptorSetLayoutBindings = {};

		VkDescriptorPool                   m_DescriptorPool       =   VK_NULL_HANDLE;   // The pool from which we allocate descriptors
		VkDescriptorSetLayout              m_DescriptorSetLayout  = { VK_NULL_HANDLE }; // The layout of the shader descriptor bindings
		std::vector<VkDescriptorSet>       m_DescriptorSets       = { VK_NULL_HANDLE }; // The actual data to bind into each descriptor layout slot

		uint32_t m_NumBufferDescriptors = 0;
		uint32_t m_NumImageDescriptors  = 0;

	};

};