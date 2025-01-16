#pragma once

#include <vulkan/vulkan.h>
#include <GraphicsContext.hpp>

#include <array>
#include <string>
#include <vector>

namespace NVulkanEngine
{
	class CPipeline
	{
	public:
		CPipeline() = default;
		~CPipeline() = default;

		void SetVertexShader(const std::string& vertexShaderPath);
		void SetFragmentShader(const std::string& fragmentShaderPath);

		void SetVertexInput(uint32_t stride, VkVertexInputRate vertexInputRate);
		void AddVertexAttribute(uint32_t locationSlot, VkFormat format, uint32_t offset);

		void SetCullingMode(VkCullModeFlagBits cullMode);
		void AddPushConstantSlot(VkShaderStageFlags shaderStage, size_t constantsSize, size_t offset);
		void AddColorAttachment(VkFormat colorFormat);
		void AddDepthAttachment(VkFormat depthFormat);

		void CreatePipeline(CGraphicsContext* context, VkDescriptorSetLayout descriptorSetLayout);

		VkPipelineLayout GetPipelineLayout() { return m_PipelineLayout; };

		void BindPipeline(VkCommandBuffer commandBuffer);
		void PushConstants(VkCommandBuffer, void* pushConstantData);

		void Cleanup(CGraphicsContext* context);
	private:
		void CreateGraphicsPipeline(CGraphicsContext* context, VkDescriptorSetLayout descriptorSetLayout);

		VkVertexInputBindingDescription m_VertexInputBindingDescription = {};
		std::vector<VkVertexInputAttributeDescription> m_VertexAttributeDescriptions    = {};

		std::string           m_VertexShaderPath       = "";
		std::string           m_FragmentShaderPath     = "";
		std::string           m_ComputeShaderPath      = "";

		VkFormat              m_DepthAttachmentFormat  = VK_FORMAT_UNDEFINED;
		std::vector<VkFormat> m_ColorAttachmentFormats = {};

		VkCullModeFlagBits    m_CullMode               = VK_CULL_MODE_BACK_BIT;

		VkPushConstantRange   m_PushConstantsRanges    = {};

		VkPipelineLayout      m_PipelineLayout         = VK_NULL_HANDLE;
		VkPipeline		      m_Pipeline               = VK_NULL_HANDLE;
	};

};