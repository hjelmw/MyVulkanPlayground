#pragma once

#include <vulkan/vulkan.h>
#include <GraphicsContext.hpp>

#include <array>
#include <string>
#include <vector>

namespace NVulkanEngine
{
	enum EPipelineType
	{
		EGraphicsPipeline = 0,
		EComputePipeline  = 1
	};

	class CPipeline
	{
	public:
		CPipeline(EPipelineType type) :
		m_PipelineType(type)
		{};
		~CPipeline() = default;

		void SetVertexShader(const std::string& vertexShaderPath);
		void SetFragmentShader(const std::string& fragmentShaderPath);
		void AddPushConstantSlot(VkShaderStageFlags shaderStage, size_t constantsSize, size_t offset);
		void SetCullingMode(VkCullModeFlagBits cullMode);

		void CreatePipeline(
			CGraphicsContext*                              context,
			VkPipelineLayout&                              pipelineLayout,
			VkDescriptorSetLayout&                         descriptorSetLayout,
			std::vector<VkDescriptorSetLayoutBinding>      descriptorSetLayoutBindings,
			VkVertexInputBindingDescription                vertexInputDescription,
			std::vector<VkVertexInputAttributeDescription> vertexAttributeDescription,
			std::vector<VkFormat>                          colorAttachmentFormats,
			VkFormat                                       depthFormat);


		void Bind(VkCommandBuffer commandBuffer);

		void Cleanup(CGraphicsContext* context);
	private:
		void CreateGraphicsPipeline(
			CGraphicsContext*                              context,
			VkPipelineLayout&                              pipelineLayout,
			VkDescriptorSetLayout&                         descriptorSetLayout,
			std::vector<VkDescriptorSetLayoutBinding>      descriptorSetLayoutBindings,
			VkVertexInputBindingDescription                vertexInputDescription,
			std::vector<VkVertexInputAttributeDescription> vertexAttributeDescription,
			std::vector<VkFormat>                          colorAttachmentFormats,
			VkFormat                                       depthFormat,
			const std::string                              vertexShaderPath, 
			const std::string                              fragmentShaderPath);

		// TODO
		/*
		void CreateComputePipeline(
			VkRenderPass renderPass,
			VkPipelineLayout pipelineLayout,
			VkDescriptorSetLayout descriptorSetLayout,
			const std::string computeShaderPath);
		*/

		EPipelineType                    m_PipelineType        = EGraphicsPipeline;
		VkCullModeFlagBits               m_CullMode            = VK_CULL_MODE_BACK_BIT;

		std::string                      m_VertexShaderPath    = "";
		std::string                      m_FragmentShaderPath  = "";
		std::string                      m_ComputeShaderPath   = "";

		VkPipeline		                 m_Pipeline            = VK_NULL_HANDLE;

		std::vector<VkPushConstantRange> m_PushConstantsRanges = {};

	};

};