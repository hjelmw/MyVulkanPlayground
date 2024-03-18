#include "LightingPass.hpp"

// GBuffer attachments
#include "GeometryPass.hpp"
#include "ShadowPass.hpp"
#include "../InputManager.hpp"

namespace NVulkanEngine
{
	enum ELightingAttachments
	{
		EPositions                = 0,
		ENormals                  = 1,
		EAlbedo                   = 2,
		EDepth                    = 3,
		EShadowMap                = 4,
		ESwapchainImage           = 5,
		ELightingAttachmentsCount = 6
	};

	struct SDeferredLightingUniformBuffer
	{
		struct Light
		{
			glm::mat4 m_LightMatrix;

			glm::vec3 m_LightPosition;
			float     m_LightRadius;

			glm::vec3 m_LightColor;
			float     m_LightIntensity;
		};

		Light     m_Lights[1];
		glm::vec3 m_ViewPos;
		float     m_Pad1;
	};

	void CLightingPass::InitPass(CGraphicsContext* context)
	{
		m_DeferredAttachments.resize(ELightingAttachmentsCount);

		m_DeferredAttachments[EPositions] = CGeometryPass::GetGBufferAttachment(EPositions);
		m_DeferredAttachments[ENormals]   = CGeometryPass::GetGBufferAttachment(ENormals);
		m_DeferredAttachments[EAlbedo]    = CGeometryPass::GetGBufferAttachment(EAlbedo);
		m_DeferredAttachments[EDepth]     = CGeometryPass::GetGBufferAttachment(EDepth);
		m_DeferredAttachments[EShadowMap] = CShadowPass::GetShadowMapAttachment();

		// Don't clear contents on BeginRendering()
		m_DeferredAttachments[EPositions].m_RenderAttachmentInfo.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
		m_DeferredAttachments[ENormals].m_RenderAttachmentInfo.loadOp   = VK_ATTACHMENT_LOAD_OP_LOAD;
		m_DeferredAttachments[EAlbedo].m_RenderAttachmentInfo.loadOp    = VK_ATTACHMENT_LOAD_OP_LOAD;
		m_DeferredAttachments[EDepth].m_RenderAttachmentInfo.loadOp     = VK_ATTACHMENT_LOAD_OP_LOAD;
		m_DeferredAttachments[EShadowMap].m_RenderAttachmentInfo.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;

		const std::vector<VkDescriptorSetLayoutBinding>      descriptorSetLayoutBindings = CLightingPass::GetDescriptorSetLayoutBindings();
		const VkVertexInputBindingDescription                vertexBindingDescription    = {};
		const std::vector<VkVertexInputAttributeDescription> vertexAttributeDescriptions = {};

		const std::vector<VkFormat> colorAttachmentFormats =
		{
			CSwapchain::GetInstance()->GetSwapchainFormat()
		};
		const VkFormat depthFormat = m_DeferredAttachments[EDepth].m_Format;

		m_DeferredPipeline = new CPipeline(EGraphicsPipeline);
		m_DeferredPipeline->SetVertexShader("shaders/deferred.vert.spv");
		m_DeferredPipeline->SetFragmentShader("shaders/deferred.frag.spv");
		m_DeferredPipeline->SetCullingMode(VK_CULL_MODE_FRONT_BIT);
		m_DeferredPipeline->CreatePipeline(
			context,
			CDrawPass::m_PipelineLayout,
			CDrawPass::m_DescriptorSetLayout,
			descriptorSetLayoutBindings,
			vertexBindingDescription,
			vertexAttributeDescriptions,
			colorAttachmentFormats,
			depthFormat);

		/* Allocat sets for 5 sampled images (pos, normals, albedo, depth, shadowmap) and 1 uniform light buffer  */
		AllocateDescriptorPool(context, g_MaxFramesInFlight, g_MaxFramesInFlight * 5, g_MaxFramesInFlight * 1);

		m_DeferredSampler = CreateSampler(context, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,       VK_SAMPLER_MIPMAP_MODE_LINEAR, VK_FILTER_NEAREST, VK_FILTER_NEAREST, 0.0f, 0.0f, 1.0f);
		m_ClampSampler    = CreateSampler(context, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER, VK_SAMPLER_MIPMAP_MODE_LINEAR, VK_FILTER_LINEAR,  VK_FILTER_LINEAR, 0.0f, 0.0f, 1.0f);

		VkDescriptorImageInfo  descriptorPositions = CreateDescriptorImageInfo(m_DeferredSampler, m_DeferredAttachments[EPositions].m_ImageView, VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL);
		VkDescriptorImageInfo  descriptorNormals   = CreateDescriptorImageInfo(m_DeferredSampler, m_DeferredAttachments[ENormals].m_ImageView,   VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL);
		VkDescriptorImageInfo  descriptorAlbedo    = CreateDescriptorImageInfo(m_DeferredSampler, m_DeferredAttachments[EAlbedo].m_ImageView,    VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL);
		VkDescriptorImageInfo  descriptorDepth     = CreateDescriptorImageInfo(m_DeferredSampler, m_DeferredAttachments[EDepth].m_ImageView,     VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);
		VkDescriptorImageInfo  descriptorShadow    = CreateDescriptorImageInfo(m_ClampSampler, m_DeferredAttachments[EShadowMap].m_ImageView, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);

		m_DeferredLightBuffer = CreateBuffer(
			context,
			m_DeferredLightBufferMemory,
			sizeof(SDeferredLightingUniformBuffer),
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

		VkDescriptorBufferInfo descriptorUniform = CreateDescriptorBufferInfo(m_DeferredLightBuffer, sizeof(SDeferredLightingUniformBuffer));

		m_DescriptorSetsLighting = AllocateDescriptorSets(context, m_DescriptorPool, CDrawPass::m_DescriptorSetLayout, g_MaxFramesInFlight);

		const std::vector<VkWriteDescriptorSet> writeDescriptorSets =
		{
			CreateWriteDescriptorImage(context,  m_DescriptorSetsLighting.data(), VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, &descriptorPositions),
			CreateWriteDescriptorImage(context,  m_DescriptorSetsLighting.data(), VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, &descriptorNormals),
			CreateWriteDescriptorImage(context,  m_DescriptorSetsLighting.data(), VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2, &descriptorAlbedo),
			CreateWriteDescriptorImage(context,  m_DescriptorSetsLighting.data(), VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 3, &descriptorDepth),
			CreateWriteDescriptorImage(context,  m_DescriptorSetsLighting.data(), VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4, &descriptorShadow),

			CreateWriteDescriptorBuffer(context, m_DescriptorSetsLighting.data(), VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         5, &descriptorUniform)
		};

		UpdateDescriptorSets(context, m_DescriptorSetsLighting, writeDescriptorSets);
	}

	void CLightingPass::UpdateLightBuffers(CGraphicsContext* context)
	{
		SDeferredLightingUniformBuffer deferredLightingUbo{};
		deferredLightingUbo.m_Lights[0].m_LightColor    = glm::vec3(1.0f, 1.0f, 1.0f);
		deferredLightingUbo.m_Lights[0].m_LightPosition = CGeometryPass::GetSphereMatrix()[3];
		deferredLightingUbo.m_Lights[0].m_LightRadius   = 20.0f;

		deferredLightingUbo.m_Lights[0].m_LightIntensity = 10.0f;
		deferredLightingUbo.m_Lights[0].m_LightMatrix = CShadowPass::GetLightMatrix();

		CCamera* camera = CInputManager::GetInstance()->GetCamera();
		deferredLightingUbo.m_ViewPos = camera->GetPosition();

		deferredLightingUbo.m_Pad1 = 0.0f;

		void* data;
		vkMapMemory(context->GetLogicalDevice(), m_DeferredLightBufferMemory, 0, sizeof(SDeferredLightingUniformBuffer), 0, &data);
		memcpy(data, &deferredLightingUbo, sizeof(SDeferredLightingUniformBuffer));
		vkUnmapMemory(context->GetLogicalDevice(), m_DeferredLightBufferMemory);
	}

	void CLightingPass::Draw(CGraphicsContext* context, VkCommandBuffer commandBuffer)
	{
		// Update with current frame swapchain image
		m_DeferredAttachments[ESwapchainImage] = CDrawPass::GetSwapchainAttachment(context);

		UpdateLightBuffers(context);

		VkImage  swapchainImage  = m_DeferredAttachments[ESwapchainImage].m_Image;
		VkFormat swapchainFormat = m_DeferredAttachments[ESwapchainImage].m_Format;
		VkImage  positionsImage  = m_DeferredAttachments[EPositions].m_Image;
		VkFormat positionsFormat = m_DeferredAttachments[EPositions].m_Format;
		VkImage  normalsImage    = m_DeferredAttachments[ENormals].m_Image;
		VkFormat normalsFormat   = m_DeferredAttachments[ENormals].m_Format;
		VkImage  albedoImage     = m_DeferredAttachments[EAlbedo].m_Image;
		VkFormat albedoFormat    = m_DeferredAttachments[EAlbedo].m_Format;
		VkImage  depthImage      = m_DeferredAttachments[EDepth].m_Image;
		VkFormat depthFormat     = m_DeferredAttachments[EDepth].m_Format;
		VkImage  ShadowImage     = m_DeferredAttachments[EShadowMap].m_Image;
		VkFormat ShadowFormat    = m_DeferredAttachments[EShadowMap].m_Format;

		TransitionImageLayout(commandBuffer, swapchainImage, swapchainFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 1);
		
		TransitionImageLayout(commandBuffer, positionsImage, positionsFormat, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,         VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 1);
		TransitionImageLayout(commandBuffer, normalsImage,   normalsFormat,   VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,         VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 1);
		TransitionImageLayout(commandBuffer, albedoImage,    albedoFormat,    VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,         VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 1);
		TransitionImageLayout(commandBuffer, depthImage,     depthFormat,     VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 1);
		TransitionImageLayout(commandBuffer, ShadowImage,    depthFormat,     VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 1);

		std::vector<SImageAttachment> swapchainAttachment = { m_DeferredAttachments[ESwapchainImage] };
		BeginRendering(context, commandBuffer, swapchainAttachment);

		m_DeferredPipeline->BindPipeline(commandBuffer);
		
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_PipelineLayout, 0, 1, &m_DescriptorSetsLighting[context->GetFrameIndex()], 0, nullptr);

		// Draw single triangle covering entire screen
		vkCmdDraw(commandBuffer, 3, 1, 0, 0);

		EndRendering(commandBuffer);

		TransitionImageLayout(commandBuffer, swapchainImage, swapchainFormat, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, 1);

		TransitionImageLayout(commandBuffer, positionsImage, positionsFormat, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 1);
		TransitionImageLayout(commandBuffer, normalsImage,   normalsFormat,   VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 1);
		TransitionImageLayout(commandBuffer, albedoImage,    albedoFormat,    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 1);
		TransitionImageLayout(commandBuffer, depthImage,     depthFormat,     VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, 1);
		TransitionImageLayout(commandBuffer, ShadowImage,    depthFormat,     VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, 1);

	}

	void CLightingPass::CleanupPass(CGraphicsContext* context)
	{
		// Uniform buffer
		vkDestroyBuffer(context->GetLogicalDevice(), m_DeferredLightBuffer, nullptr);
		vkFreeMemory(context->GetLogicalDevice(), m_DeferredLightBufferMemory, nullptr);

		// Attachments
		for (uint32_t i = 0; i < m_DeferredAttachments.size(); i++)
		{
			vkDestroyImageView(context->GetLogicalDevice(), m_DeferredAttachments[i].m_ImageView, nullptr);

			vkDestroyImage(context->GetLogicalDevice(), m_DeferredAttachments[i].m_Image, nullptr);
			vkFreeMemory(context->GetLogicalDevice(), m_DeferredAttachments[i].m_Memory, nullptr);

			vkDestroySampler(context->GetLogicalDevice(), m_DeferredSampler, nullptr);
			vkDestroySampler(context->GetLogicalDevice(), m_ClampSampler, nullptr);

			m_DeferredAttachments[i].m_Format = VK_FORMAT_UNDEFINED;
			m_DeferredAttachments[i].m_RenderAttachmentInfo = {};
		}
	}

	const std::vector<VkDescriptorSetLayoutBinding> CLightingPass::GetDescriptorSetLayoutBindings()
	{
		std::vector<VkDescriptorSetLayoutBinding> attributeDescriptions(6);

		// 0: GBuffer positions
		attributeDescriptions[0].binding = 0;
		attributeDescriptions[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		attributeDescriptions[0].descriptorCount = 1;
		attributeDescriptions[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		attributeDescriptions[0].pImmutableSamplers = nullptr;

		// 1: GBuffer normals
		attributeDescriptions[1].binding = 1;
		attributeDescriptions[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		attributeDescriptions[1].descriptorCount = 1;
		attributeDescriptions[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		attributeDescriptions[1].pImmutableSamplers = nullptr;

		// 2: GBuffer albedo
		attributeDescriptions[2].binding = 2;
		attributeDescriptions[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		attributeDescriptions[2].descriptorCount = 1;
		attributeDescriptions[2].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		attributeDescriptions[2].pImmutableSamplers = nullptr;

		// 3: GBuffer depth
		attributeDescriptions[3].binding = 3;
		attributeDescriptions[3].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		attributeDescriptions[3].descriptorCount = 1;
		attributeDescriptions[3].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		attributeDescriptions[3].pImmutableSamplers = nullptr;

		// 4: Shadowmap depth
		attributeDescriptions[4].binding = 4;
		attributeDescriptions[4].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		attributeDescriptions[4].descriptorCount = 1;
		attributeDescriptions[4].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		attributeDescriptions[4].pImmutableSamplers = nullptr;

		// 0: Vertex shader uniform buffer
		attributeDescriptions[5].binding = 5;
		attributeDescriptions[5].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		attributeDescriptions[5].descriptorCount = 1;
		attributeDescriptions[5].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		attributeDescriptions[5].pImmutableSamplers = nullptr;

		return attributeDescriptions;
	}
}