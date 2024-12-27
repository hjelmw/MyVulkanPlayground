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

	void CLightingPass::InitPass(CGraphicsContext* context, SGraphicsManagers* managers)
	{
		CAttachmentManager* attachmentManager = managers->m_AttachmentManager;

		SRenderAttachment sceneColorAttachment = attachmentManager->AddAttachment(
			context,
			"Scene Color",
			EAttachmentIndices::SceneColor,
			context->GetLinearClampSampler(),
			VK_FORMAT_R8G8B8A8_UNORM,
			VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			g_DisplayWidth,
			g_DisplayHeight);

		const std::vector<VkDescriptorSetLayoutBinding>      descriptorSetLayoutBindings = GetLightingBindings();
		const VkVertexInputBindingDescription                vertexBindingDescription    = {};
		const std::vector<VkVertexInputAttributeDescription> vertexAttributeDescriptions = {};

		const std::vector<VkFormat> colorAttachmentFormats =
		{
			sceneColorAttachment.m_Format
		};
		const VkFormat depthFormat = attachmentManager->GetAttachment(EAttachmentIndices::Depth).m_Format;

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


		VkDescriptorImageInfo  descriptorPositions    = CreateDescriptorImageInfo(context->GetLinearClampSampler(), attachmentManager->GetAttachment(EAttachmentIndices::Positions).m_ImageView,          VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		VkDescriptorImageInfo  descriptorNormals      = CreateDescriptorImageInfo(context->GetLinearClampSampler(), attachmentManager->GetAttachment(EAttachmentIndices::Normals).m_ImageView,            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		VkDescriptorImageInfo  descriptorAlbedo       = CreateDescriptorImageInfo(context->GetLinearClampSampler(), attachmentManager->GetAttachment(EAttachmentIndices::Albedo).m_ImageView,             VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		VkDescriptorImageInfo  descriptorDepth        = CreateDescriptorImageInfo(context->GetLinearClampSampler(), attachmentManager->GetAttachment(EAttachmentIndices::Depth).m_ImageView,              VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL);
		VkDescriptorImageInfo  descriptorShadow       = CreateDescriptorImageInfo(context->GetLinearClampSampler(), attachmentManager->GetAttachment(EAttachmentIndices::ShadowMap).m_ImageView,          VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		VkDescriptorImageInfo  descriptorAtmospherics = CreateDescriptorImageInfo(context->GetLinearClampSampler(), attachmentManager->GetAttachment(EAttachmentIndices::AtmosphericsSkyBox).m_ImageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

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
	}

	void CLightingPass::UpdateLightBuffers(CGraphicsContext* context, SGraphicsManagers* managers)
	{
		SDeferredLightingUniformBuffer deferredLightingUbo{};
		deferredLightingUbo.m_Lights[0].m_LightColor    = glm::vec3(1.0f, 1.0f, 1.0f);
		deferredLightingUbo.m_Lights[0].m_LightPosition = CGeometryPass::GetSphereMatrix()[3];
		deferredLightingUbo.m_Lights[0].m_LightRadius   = g_LightRadius;

		deferredLightingUbo.m_Lights[0].m_LightIntensity = 100.0f;
		deferredLightingUbo.m_Lights[0].m_LightMatrix = CShadowPass::GetLightMatrix();

		CCamera* camera = managers->m_InputManager->GetCamera();
		deferredLightingUbo.m_ViewPos = camera->GetPosition();

		deferredLightingUbo.m_Pad1 = 0.0f;

		void* data;
		vkMapMemory(context->GetLogicalDevice(), m_DeferredLightBufferMemory, 0, sizeof(SDeferredLightingUniformBuffer), 0, &data);
		memcpy(data, &deferredLightingUbo, sizeof(SDeferredLightingUniformBuffer));
		vkUnmapMemory(context->GetLogicalDevice(), m_DeferredLightBufferMemory);
	}

	void CLightingPass::Draw(CGraphicsContext* context, SGraphicsManagers* managers, VkCommandBuffer commandBuffer)
	{
		UpdateLightBuffers(context, managers);

		CAttachmentManager* attachmentManager = managers->m_AttachmentManager;
		attachmentManager->TransitionAttachment(commandBuffer, EAttachmentIndices::Positions,          VK_ATTACHMENT_LOAD_OP_LOAD, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		attachmentManager->TransitionAttachment(commandBuffer, EAttachmentIndices::Normals,            VK_ATTACHMENT_LOAD_OP_LOAD, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		attachmentManager->TransitionAttachment(commandBuffer, EAttachmentIndices::Albedo,             VK_ATTACHMENT_LOAD_OP_LOAD, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		attachmentManager->TransitionAttachment(commandBuffer, EAttachmentIndices::Depth,              VK_ATTACHMENT_LOAD_OP_LOAD, VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL);
		attachmentManager->TransitionAttachment(commandBuffer, EAttachmentIndices::ShadowMap,          VK_ATTACHMENT_LOAD_OP_LOAD, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		attachmentManager->TransitionAttachment(commandBuffer, EAttachmentIndices::AtmosphericsSkyBox, VK_ATTACHMENT_LOAD_OP_LOAD, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		SRenderAttachment sceneColorAttachment = attachmentManager->TransitionAttachment(commandBuffer, EAttachmentIndices::SceneColor, VK_ATTACHMENT_LOAD_OP_LOAD, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

		std::vector<SRenderAttachment> sceneColorAttachments = { sceneColorAttachment };
		BeginRendering(context, commandBuffer, sceneColorAttachments);

		m_DeferredPipeline->Bind(commandBuffer);
		
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_PipelineLayout, 0, 1, &m_DescriptorSetsLighting[context->GetFrameIndex()], 0, nullptr);

		// Draw single triangle covering entire screen
		vkCmdDraw(commandBuffer, 3, 1, 0, 0);

		EndRendering(commandBuffer);
	}

	void CLightingPass::CleanupPass(CGraphicsContext* context)
	{
		// Uniform buffer
		vkDestroyBuffer(context->GetLogicalDevice(), m_DeferredLightBuffer, nullptr);
		vkFreeMemory(context->GetLogicalDevice(), m_DeferredLightBufferMemory, nullptr);
		
		m_DeferredPipeline->Cleanup(context);
		delete m_DeferredPipeline;
	}
}