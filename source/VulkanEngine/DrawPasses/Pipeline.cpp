#include "Pipeline.hpp"

#include <GraphicsContext.hpp>
#include <VulkanGraphicsEngineUtils.hpp>

#include <array>
#include <string>

namespace NVulkanEngine
{
	void CPipeline::SetVertexShader(const std::string& vertexShaderPath)
	{
		m_VertexShaderPath = vertexShaderPath;
	}
	void CPipeline::SetFragmentShader(const std::string& fragmentShaderPath)
	{
		m_FragmentShaderPath = fragmentShaderPath;
	}

	void CPipeline::SetCullingMode(VkCullModeFlagBits cullMode)
	{
		m_CullMode = cullMode;
	}

	void CPipeline::AddPushConstantSlot(VkShaderStageFlags shaderStage, size_t constantsSize, size_t offset)
	{
		VkPushConstantRange pushConstant;
		pushConstant.stageFlags = shaderStage;
		pushConstant.offset     = static_cast<uint32_t>(offset);
		pushConstant.size       = static_cast<uint32_t>(constantsSize);

		m_PushConstantsRanges = pushConstant;
	}

	void CPipeline::SetVertexInput(uint32_t stride, VkVertexInputRate vertexInputRate)
	{
		VkVertexInputBindingDescription vertexInputBindingDescription{};
		vertexInputBindingDescription.binding   = 0;
		vertexInputBindingDescription.stride    = stride;
		vertexInputBindingDescription.inputRate = vertexInputRate;

		m_VertexInputBindingDescription = vertexInputBindingDescription;
	}

	void CPipeline::AddVertexAttribute(uint32_t locationSlot, VkFormat format, uint32_t offset)
	{
		VkVertexInputAttributeDescription vertexAttributeDescription{};
		vertexAttributeDescription.binding  = 0;
		vertexAttributeDescription.location = locationSlot;
		vertexAttributeDescription.format   = format;
		vertexAttributeDescription.offset   = offset;
		
		m_VertexAttributeDescriptions.push_back(vertexAttributeDescription);
	}

	void CPipeline::AddColorAttachment(VkFormat colorFormat)
	{
		m_ColorAttachmentFormats.push_back(colorFormat);
	}

	void CPipeline::AddDepthAttachment(VkFormat depthFormat)
	{
		m_DepthAttachmentFormat = depthFormat;
	}

	void CPipeline::CreatePipeline(CGraphicsContext* context, VkDescriptorSetLayout descriptorSetLayout)
	{
		CreateGraphicsPipeline(context, descriptorSetLayout);
#if defined(_DEBUG)
		std::cout << "\n --- Pipeline created succesfully! ---" << "\n" << std::endl;
#endif
	}

	void CPipeline::CreateGraphicsPipeline(CGraphicsContext* context, VkDescriptorSetLayout descriptorSetLayout)
	{
#if defined(_DEBUG)
		std::cout << "\n --- Creating Graphics pipeline ---" << "\n" << std::endl;
#endif

		VkShaderModule vertexShaderModule   = CreateShaderModule(context->GetLogicalDevice(), m_VertexShaderPath);
		VkShaderModule fragmentShaderModule = CreateShaderModule(context->GetLogicalDevice(), m_FragmentShaderPath);

		VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
		vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
		vertShaderStageInfo.module = vertexShaderModule;
		vertShaderStageInfo.pName = "main";

		VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
		fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		fragShaderStageInfo.module = fragmentShaderModule;
		fragShaderStageInfo.pName = "main";

		VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

		VkPipelineVertexInputStateCreateInfo vertexInputInfo{};

		vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInputInfo.vertexBindingDescriptionCount = 1; // single binding descriptor for now
		vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(m_VertexAttributeDescriptions.size());
		vertexInputInfo.pVertexBindingDescriptions = &m_VertexInputBindingDescription;
		vertexInputInfo.pVertexAttributeDescriptions = m_VertexAttributeDescriptions.data();

		VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
		inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		inputAssembly.primitiveRestartEnable = VK_FALSE;

		VkPipelineViewportStateCreateInfo viewportState{};
		viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportState.viewportCount = 1;
		viewportState.scissorCount = 1;

		VkPipelineRasterizationStateCreateInfo rasterizer{};
		rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizer.depthClampEnable = VK_FALSE;
		rasterizer.rasterizerDiscardEnable = VK_FALSE;
		rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
		rasterizer.lineWidth = 1.0f;
		rasterizer.depthBiasEnable = VK_FALSE;

#if defined(_DEBUG)
		std::cout << "Cull mode: ";
#endif
		switch(m_CullMode)
		{
		case VK_CULL_MODE_FRONT_BIT:
#if defined(_DEBUG)
			std::cout << "Frontface " << std::endl;
#endif
			break;
		case VK_CULL_MODE_BACK_BIT:
#if defined(_DEBUG)
			std::cout << "Backface " << std::endl;
#endif
			break;
		}
		rasterizer.cullMode = m_CullMode;

		rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

		VkPipelineMultisampleStateCreateInfo multisampling{};
		multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampling.sampleShadingEnable = VK_FALSE;
		multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

		std::vector<VkPipelineColorBlendAttachmentState> colorBlendAttachments(m_ColorAttachmentFormats.size());

#if defined(_DEBUG)
		std::cout << "Attachments  " << "\n\tColor: " << m_ColorAttachmentFormats.size() << "\n\tDepth: 1" << std::endl;
#endif
		for (uint32_t i = 0; i < m_ColorAttachmentFormats.size(); i++)
		{
			colorBlendAttachments[i].colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
			colorBlendAttachments[i].blendEnable = VK_FALSE;
		}

		VkPipelineColorBlendStateCreateInfo colorBlending{};
		colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlending.logicOpEnable = VK_FALSE;
		colorBlending.logicOp = VK_LOGIC_OP_COPY;
		colorBlending.attachmentCount = static_cast<uint32_t>(colorBlendAttachments.size());
		colorBlending.pAttachments = colorBlendAttachments.data();
		colorBlending.blendConstants[0] = 0.0f;
		colorBlending.blendConstants[1] = 0.0f;
		colorBlending.blendConstants[2] = 0.0f;
		colorBlending.blendConstants[3] = 0.0f;

		std::vector<VkDynamicState> dynamicStates =
		{
			VK_DYNAMIC_STATE_VIEWPORT,
			VK_DYNAMIC_STATE_SCISSOR
		};

		VkPipelineDynamicStateCreateInfo dynamicState{};
		dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
		dynamicState.pDynamicStates = dynamicStates.data();

		VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = 1;
		pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;
		pipelineLayoutInfo.pushConstantRangeCount = m_PushConstantsRanges.size != 0 ? 1 : 0; // For now
		pipelineLayoutInfo.pPushConstantRanges = &m_PushConstantsRanges;

		if (vkCreatePipelineLayout(context->GetLogicalDevice(), &pipelineLayoutInfo, nullptr, &m_PipelineLayout) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create pipeline layout!");
		}

		VkPipelineDepthStencilStateCreateInfo depthStencil{};
		depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		depthStencil.depthTestEnable = VK_TRUE;
		depthStencil.depthWriteEnable = VK_TRUE;
		depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
		depthStencil.depthBoundsTestEnable = VK_FALSE;
		depthStencil.minDepthBounds = 0.0f; // Optional
		depthStencil.maxDepthBounds = 1.0f; // Optional
		depthStencil.stencilTestEnable = VK_FALSE;
		depthStencil.front = {}; // Optional
		depthStencil.back = {}; // Optional

		VkPipelineRenderingCreateInfo renderingCreateInfo{};	
		renderingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR;
		renderingCreateInfo.colorAttachmentCount = static_cast<uint32_t>(m_ColorAttachmentFormats.size());
		renderingCreateInfo.pColorAttachmentFormats = m_ColorAttachmentFormats.data();
		renderingCreateInfo.depthAttachmentFormat = m_DepthAttachmentFormat;

		VkGraphicsPipelineCreateInfo pipelineInfo{};
		pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineInfo.stageCount = 2;
		pipelineInfo.pStages = shaderStages;
		pipelineInfo.pVertexInputState = &vertexInputInfo;
		pipelineInfo.pInputAssemblyState = &inputAssembly;
		pipelineInfo.pViewportState = &viewportState;
		pipelineInfo.pRasterizationState = &rasterizer;
		pipelineInfo.pMultisampleState = &multisampling;
		pipelineInfo.pColorBlendState = &colorBlending;
		pipelineInfo.pDynamicState = &dynamicState;
		pipelineInfo.pDepthStencilState = &depthStencil;
		pipelineInfo.layout = m_PipelineLayout;
		pipelineInfo.renderPass = VK_NULL_HANDLE;  // Don't need this since we are using dynamic rendering :)
		pipelineInfo.pNext = &renderingCreateInfo; // Set this instead!
		pipelineInfo.subpass = 0;
		pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

		if (vkCreateGraphicsPipelines(context->GetLogicalDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_Pipeline) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create graphics pipeline!");
		}

		vkDestroyShaderModule(context->GetLogicalDevice(), vertexShaderModule, nullptr);
		vkDestroyShaderModule(context->GetLogicalDevice(), fragmentShaderModule, nullptr);
	}

	void CPipeline::BindPipeline(VkCommandBuffer commandBuffer)
	{
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_Pipeline);
	}

	void CPipeline::PushConstants(VkCommandBuffer commandBuffer, void* data)
	{
		vkCmdPushConstants(commandBuffer, m_PipelineLayout, m_PushConstantsRanges.stageFlags, m_PushConstantsRanges.offset, m_PushConstantsRanges.size, { data });
	}

	void CPipeline::Cleanup(CGraphicsContext* context)
	{
		vkDestroyPipeline(context->GetLogicalDevice(), m_Pipeline, nullptr);
		vkDestroyPipelineLayout(context->GetLogicalDevice(), m_PipelineLayout, nullptr);
	}
}