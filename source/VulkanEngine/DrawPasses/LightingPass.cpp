#include "LightingPass.hpp"

// GBuffer attachments
#include <DrawPasses/GeometryPass.hpp>
#include <DrawPasses/ShadowPass.hpp>
#include <DrawPasses/AtmosphericsPass.hpp>

#include <Managers/InputManager.hpp>

#include <imgui.h>
#include <backends/imgui_impl_vulkan.h>

static float g_LightRadius = 1500.0f;

namespace NVulkanEngine
{
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

	const std::vector<VkDescriptorSetLayoutBinding> GetLightingBindings()
	{
		std::vector<VkDescriptorSetLayoutBinding> attributeDescriptions(7);

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

		// 5: Atmpspherics skybox
		attributeDescriptions[5].binding = 5;
		attributeDescriptions[5].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		attributeDescriptions[5].descriptorCount = 1;
		attributeDescriptions[5].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		attributeDescriptions[5].pImmutableSamplers = nullptr;

		// 0: Vertex shader uniform buffer
		attributeDescriptions[6].binding = 6;
		attributeDescriptions[6].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		attributeDescriptions[6].descriptorCount = 1;
		attributeDescriptions[6].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		attributeDescriptions[6].pImmutableSamplers = nullptr;

		return attributeDescriptions;
	}

	void CLightingPass::InitPass(CGraphicsContext* context)
	{
		s_DeferredAttachments.resize((uint32_t)ERenderAttachments::Count);

		s_DeferredAttachments[(uint32_t)ERenderAttachments::SceneColor] = CreateRenderAttachment(
			context,
			VK_FORMAT_R8G8B8A8_UNORM,
			VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			context->GetRenderResolution().width,
			context->GetRenderResolution().height);

		s_DeferredAttachments[0] = CGeometryPass::GetGBufferAttachment(ERenderAttachments::Positions);
		s_DeferredAttachments[1] = CGeometryPass::GetGBufferAttachment(ERenderAttachments::Normals);
		s_DeferredAttachments[2] = CGeometryPass::GetGBufferAttachment(ERenderAttachments::Albedo);
		s_DeferredAttachments[3] = CGeometryPass::GetGBufferAttachment(ERenderAttachments::Depth);
		s_DeferredAttachments[4] = CShadowPass::GetShadowMapAttachment();
		s_DeferredAttachments[6] = CAtmosphericsPass::GetAtmosphericsAttachment();

		// Don't clear contents on BeginRendering()
		s_DeferredAttachments[0].m_RenderAttachmentInfo.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
		s_DeferredAttachments[1].m_RenderAttachmentInfo.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
		s_DeferredAttachments[2].m_RenderAttachmentInfo.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
		s_DeferredAttachments[3].m_RenderAttachmentInfo.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
		s_DeferredAttachments[4].m_RenderAttachmentInfo.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
		s_DeferredAttachments[6].m_RenderAttachmentInfo.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;

		const std::vector<VkDescriptorSetLayoutBinding>      descriptorSetLayoutBindings = GetLightingBindings();
		const VkVertexInputBindingDescription                vertexBindingDescription    = {};
		const std::vector<VkVertexInputAttributeDescription> vertexAttributeDescriptions = {};

		const std::vector<VkFormat> colorAttachmentFormats =
		{
			s_DeferredAttachments[(uint32_t)ERenderAttachments::SceneColor].m_Format
		};
		const VkFormat depthFormat = s_DeferredAttachments[(uint32_t)ERenderAttachments::Depth].m_Format;

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
			
		/* Allocat sets for 6 sampled images (pos, normals, albedo, depth, shadowmap, atmospherics) and 1 uniform light buffer  */
		AllocateDescriptorPool(context, g_MaxFramesInFlight, g_MaxFramesInFlight * 6, g_MaxFramesInFlight * 1);

		m_DeferredSampler = CreateSampler(context, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,       VK_SAMPLER_MIPMAP_MODE_LINEAR, VK_FILTER_NEAREST, VK_FILTER_NEAREST, 0.0f, 0.0f, 1.0f);
		m_ClampSampler    = CreateSampler(context, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER, VK_SAMPLER_MIPMAP_MODE_LINEAR, VK_FILTER_LINEAR,  VK_FILTER_LINEAR, 0.0f, 0.0f, 1.0f);

		VkDescriptorImageInfo  descriptorPositions    = CreateDescriptorImageInfo(m_DeferredSampler, s_DeferredAttachments[0].m_ImageView, VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL);
		VkDescriptorImageInfo  descriptorNormals      = CreateDescriptorImageInfo(m_DeferredSampler, s_DeferredAttachments[1].m_ImageView, VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL);
		VkDescriptorImageInfo  descriptorAlbedo       = CreateDescriptorImageInfo(m_DeferredSampler, s_DeferredAttachments[2].m_ImageView, VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL);
		VkDescriptorImageInfo  descriptorDepth        = CreateDescriptorImageInfo(m_DeferredSampler, s_DeferredAttachments[3].m_ImageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		VkDescriptorImageInfo  descriptorShadow       = CreateDescriptorImageInfo(m_ClampSampler,    s_DeferredAttachments[4].m_ImageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		VkDescriptorImageInfo  descriptorAtmospherics = CreateDescriptorImageInfo(m_DeferredSampler, s_DeferredAttachments[6].m_ImageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

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
			CreateWriteDescriptorImage(context,  m_DescriptorSetsLighting.data(), VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 5, &descriptorAtmospherics),

			CreateWriteDescriptorBuffer(context, m_DescriptorSetsLighting.data(), VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         6, &descriptorUniform)
		};

		UpdateDescriptorSets(context, m_DescriptorSetsLighting, writeDescriptorSets);

		m_ImGuiSceneColorDescriptorSet = ImGui_ImplVulkan_AddTexture(m_DeferredSampler, s_DeferredAttachments[(uint32_t)ERenderAttachments::SceneColor].m_ImageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	}

	void CLightingPass::UpdateLightBuffers(CGraphicsContext* context)
	{
		SDeferredLightingUniformBuffer deferredLightingUbo{};
		deferredLightingUbo.m_Lights[0].m_LightColor    = glm::vec3(1.0f, 1.0f, 1.0f);
		deferredLightingUbo.m_Lights[0].m_LightPosition = CGeometryPass::GetSphereMatrix()[3];
		deferredLightingUbo.m_Lights[0].m_LightRadius   = g_LightRadius;

		deferredLightingUbo.m_Lights[0].m_LightIntensity = 100.0f;
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
		UpdateLightBuffers(context);

		SRenderAttachment positionsAttachment    = s_DeferredAttachments[(uint32_t)ERenderAttachments::Positions];
		SRenderAttachment normalsAttachment      = s_DeferredAttachments[(uint32_t)ERenderAttachments::Normals];
		SRenderAttachment albedoAttachment       = s_DeferredAttachments[(uint32_t)ERenderAttachments::Albedo];
		SRenderAttachment depthAttachment        = s_DeferredAttachments[(uint32_t)ERenderAttachments::Depth];
		SRenderAttachment shadowAttachment       = s_DeferredAttachments[(uint32_t)ERenderAttachments::ShadowMap];
		SRenderAttachment sceneColorAttachment   = s_DeferredAttachments[(uint32_t)ERenderAttachments::SceneColor];
		SRenderAttachment atmosphericsAttachment = s_DeferredAttachments[(uint32_t)ERenderAttachments::AtmosphericsSkyBox];

		TransitionImageLayout(commandBuffer, positionsAttachment,    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 1);
		TransitionImageLayout(commandBuffer, normalsAttachment,      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 1);
		TransitionImageLayout(commandBuffer, albedoAttachment,       VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 1);
		TransitionImageLayout(commandBuffer, depthAttachment,        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 1);
		TransitionImageLayout(commandBuffer, shadowAttachment,       VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 1);
		TransitionImageLayout(commandBuffer, atmosphericsAttachment, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 1);
		TransitionImageLayout(commandBuffer, sceneColorAttachment,   VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 1);

		std::vector<SRenderAttachment> sceneColorAttachments = { sceneColorAttachment };
		BeginRendering(context, commandBuffer, sceneColorAttachments);

		m_DeferredPipeline->Bind(commandBuffer);
		
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_PipelineLayout, 0, 1, &m_DescriptorSetsLighting[context->GetFrameIndex()], 0, nullptr);

		// Draw single triangle covering entire screen
		vkCmdDraw(commandBuffer, 3, 1, 0, 0);

		EndRendering(commandBuffer);

		TransitionImageLayout(commandBuffer, positionsAttachment,    VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 1);
		TransitionImageLayout(commandBuffer, normalsAttachment,      VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 1);
		TransitionImageLayout(commandBuffer, albedoAttachment,       VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 1);
		TransitionImageLayout(commandBuffer, depthAttachment,        VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL, 1);
		TransitionImageLayout(commandBuffer, shadowAttachment,       VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, 1);
		TransitionImageLayout(commandBuffer, atmosphericsAttachment, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 1);
		TransitionImageLayout(commandBuffer, sceneColorAttachment,   VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 1);

	}

	void CLightingPass::CleanupPass(CGraphicsContext* context)
	{
		// Uniform buffer
		vkDestroyBuffer(context->GetLogicalDevice(), m_DeferredLightBuffer, nullptr);
		vkFreeMemory(context->GetLogicalDevice(), m_DeferredLightBufferMemory, nullptr);

		vkDestroySampler(context->GetLogicalDevice(), m_DeferredSampler, nullptr);
		vkDestroySampler(context->GetLogicalDevice(), m_ClampSampler, nullptr);

		// Clear Attachments
		for (uint32_t i = 0; i < s_DeferredAttachments.size(); i++)
		{
			s_DeferredAttachments[i].m_Format = VK_FORMAT_UNDEFINED;
			s_DeferredAttachments[i].m_RenderAttachmentInfo = {};
		}
	}


}