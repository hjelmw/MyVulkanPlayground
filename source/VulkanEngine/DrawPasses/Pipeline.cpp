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

		m_PushConstantsRanges.push_back(pushConstant);
	}

	void CPipeline::CreatePipeline(
		CGraphicsContext*                              context,
		VkPipelineLayout&                              pipelineLayout,
		VkDescriptorSetLayout&                         descriptorSetLayout,
		std::vector<VkDescriptorSetLayoutBinding>      descriptorSetLayoutBindings,
		VkVertexInputBindingDescription                vertexInputDescription,
		std::vector<VkVertexInputAttributeDescription> vertexAttributeDescription,
		std::vector<VkFormat>                          colorAttachmentFormats,
		VkFormat                                       depthFormat)
	{
		if (m_PipelineType == EGraphicsPipeline)
		{
			CreateGraphicsPipeline(
				context,
				pipelineLayout,
				descriptorSetLayout,
				descriptorSetLayoutBindings,
				vertexInputDescription,
				vertexAttributeDescription,
				colorAttachmentFormats,
				depthFormat,
				m_VertexShaderPath,
				m_FragmentShaderPath);
		}
		else 
		{
			throw std::runtime_error("Pipeline type not supported!");
		}


#if defined(_DEBUG)
		std::cout << "\n --- Pipeline created succesfully! ---" << "\n" << std::endl;
#endif
	}

	void CPipeline::CreateGraphicsPipeline(
		CGraphicsContext*                              context,
		VkPipelineLayout&                              pipelineLayout,
		VkDescriptorSetLayout&                         descriptorSetLayout,
		std::vector<VkDescriptorSetLayoutBinding>      descriptorSetLayoutBindings,
		VkVertexInputBindingDescription                vertexInputDescription,
		std::vector<VkVertexInputAttributeDescription> vertexAttributeDescription,
		std::vector<VkFormat>                          colorAttachmentFormats,
		VkFormat                                       depthFormat,
		std::string                                    vertexShaderPath,
		std::string                                    fragmentShaderPath)
	{
#if defined(_DEBUG)
		std::cout << "\n --- Creating Graphics pipeline ---" << "\n" << std::endl;
#endif

		VkShaderModule vertexShaderModule   = CreateShaderModule(context->GetLogicalDevice(), vertexShaderPath);
		VkShaderModule fragmentShaderModule = CreateShaderModule(context->GetLogicalDevice(), fragmentShaderPath);

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
		vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertexAttributeDescription.size());
		vertexInputInfo.pVertexBindingDescriptions = &vertexInputDescription;
		vertexInputInfo.pVertexAttributeDescriptions = vertexAttributeDescription.data();

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

		std::vector<VkPipelineColorBlendAttachmentState> colorBlendAttachments(colorAttachmentFormats.size());

#if defined(_DEBUG)
		std::cout << "Attachments  " << "\n\tColor: " << colorAttachmentFormats.size() << "\n\tDepth: 1" << std::endl;
#endif
		for (uint32_t i = 0; i < colorAttachmentFormats.size(); i++)
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

		VkDescriptorSetLayoutCreateInfo layoutInfo{};
		layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutInfo.bindingCount = static_cast<uint32_t>(descriptorSetLayoutBindings.size());
		layoutInfo.pBindings = descriptorSetLayoutBindings.data();

		if (vkCreateDescriptorSetLayout(context->GetLogicalDevice(), &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create descriptor set layout!");
		}

		VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = 1;
		pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;
		pipelineLayoutInfo.pushConstantRangeCount = static_cast<uint32_t>(m_PushConstantsRanges.size());
		pipelineLayoutInfo.pPushConstantRanges = m_PushConstantsRanges.data();

		if (vkCreatePipelineLayout(context->GetLogicalDevice(), &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS)
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
		renderingCreateInfo.colorAttachmentCount = static_cast<uint32_t>(colorAttachmentFormats.size());
		renderingCreateInfo.pColorAttachmentFormats = colorAttachmentFormats.data();
		renderingCreateInfo.depthAttachmentFormat = depthFormat;

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
		pipelineInfo.layout = pipelineLayout;
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

	void CPipeline::Bind(VkCommandBuffer commandBuffer)
	{
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_Pipeline);
	}

	void CPipeline::Cleanup(CGraphicsContext* context)
	{
		vkDestroyPipeline(context->GetLogicalDevice(), m_Pipeline, nullptr);
	}
}