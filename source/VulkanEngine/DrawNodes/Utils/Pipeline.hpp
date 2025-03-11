#pragma once

#include <vulkan/vulkan.h>
#include <GraphicsContext.hpp>
#include <DrawNodes/Utils/BindingTable.hpp>

#include <array>
#include <string>
#include <vector>

/*
	Just a wrapper for simplifying creation of the VkPipeline
*/

enum class EPipelineType
{
	GRAPHICS = 0,
	COMPUTE = 1, // Not implemented yet
	COUNT = 2,
};

namespace NVulkanEngine
{
	class CPipeline
	{
	public:
		CPipeline(EPipelineType type);
		~CPipeline() = default;

		// Shaders
		void SetVertexShader(const std::string& vertexShaderPath);
		void SetFragmentShader(const std::string& fragmentShaderPath);

		// Vertex info
		void SetVertexInput(uint32_t stride, VkVertexInputRate vertexInputRate);
		void AddVertexAttribute(uint32_t locationSlot, VkFormat format, uint32_t offset);

		// Pipeline states
		void SetPrimitiveTopology(VkPrimitiveTopology primitiveTopology);
		void SetCullingMode(VkCullModeFlagBits cullMode);
	
		// Shader inputs
		void AddSampledImageBinding(uint32_t bindingSlot, VkShaderStageFlagBits shaderStage, VkImageView imageView, VkFormat format, VkSampler sampler);
		void AddSampledBufferBinding(uint32_t bindingSlot, VkShaderStageFlagBits shaderStage, VkBuffer buffer, uint32_t bufferSize);
		void AddPushConstantSlot(VkShaderStageFlags shaderStage, size_t constantsSize, size_t offset);

		// Shader outputs
		void AddColorAttachment(VkFormat colorFormat);
		void AddDepthAttachment(VkFormat depthFormat);

		void CreatePipeline(CGraphicsContext* context);
		void CreatePipeline(CGraphicsContext* context, VkDescriptorSetLayout descriptorSetLayout);

		VkPipelineLayout GetPipelineLayout() { return m_PipelineLayout; };

		void BindPipeline(CGraphicsContext* context, VkCommandBuffer commandBuffer);
		void BindPipeline(VkCommandBuffer commandBuffer);
		void PushConstants(VkCommandBuffer, void* pushConstantData);

		void Cleanup(CGraphicsContext* context);
	private:
		EPipelineType m_Type = EPipelineType::COUNT;

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

		VkPrimitiveTopology   m_PrimitiveTopology      = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

		VkPipelineLayout      m_PipelineLayout         = VK_NULL_HANDLE;
		VkPipeline		      m_Pipeline               = VK_NULL_HANDLE;

		CBindingTable*        m_BindingTable           = nullptr;
	};

};