#include "LightingNode.hpp"
#include "ShadowNode.hpp"

#include <imgui.h>

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

	void CLightingNode::Init(CGraphicsContext* context, SGraphicsManagers* managers)
	{
		const SRenderResource positionsAttachment    = managers->m_ResourceManager->GetResource(EResourceIndices::Positions);
		const SRenderResource normalsAttachment      = managers->m_ResourceManager->GetResource(EResourceIndices::Normals);
		const SRenderResource albedoAttachment       = managers->m_ResourceManager->GetResource(EResourceIndices::Albedo);
		const SRenderResource depthAttachment        = managers->m_ResourceManager->GetResource(EResourceIndices::Depth);
		const SRenderResource shadowMapAttachment    = managers->m_ResourceManager->GetResource(EResourceIndices::ShadowMap);
		const SRenderResource atmosphericsAttachment = managers->m_ResourceManager->GetResource(EResourceIndices::AtmosphericsSkyBox);

		m_DeferredUniformBuffer = CreateUniformBuffer(context, m_DeferredLightBufferMemory, sizeof(SDeferredLightingUniformBuffer));

		m_DeferredTable = new CBindingTable();
		m_DeferredTable->AddSampledImageBinding(0,  VK_SHADER_STAGE_FRAGMENT_BIT, positionsAttachment.m_ImageView,    positionsAttachment.m_Format,    context->GetLinearClampSampler());
		m_DeferredTable->AddSampledImageBinding(1,  VK_SHADER_STAGE_FRAGMENT_BIT, normalsAttachment.m_ImageView,      normalsAttachment.m_Format,      context->GetLinearClampSampler());
		m_DeferredTable->AddSampledImageBinding(2,  VK_SHADER_STAGE_FRAGMENT_BIT, albedoAttachment.m_ImageView,       albedoAttachment.m_Format,       context->GetLinearClampSampler());
		m_DeferredTable->AddSampledImageBinding(3,  VK_SHADER_STAGE_FRAGMENT_BIT, depthAttachment.m_ImageView,        depthAttachment.m_Format,        context->GetLinearClampSampler());
		m_DeferredTable->AddSampledImageBinding(4,  VK_SHADER_STAGE_FRAGMENT_BIT, shadowMapAttachment.m_ImageView,    shadowMapAttachment.m_Format,    context->GetLinearClampSampler());
		m_DeferredTable->AddSampledImageBinding(5,  VK_SHADER_STAGE_FRAGMENT_BIT, atmosphericsAttachment.m_ImageView, atmosphericsAttachment.m_Format, context->GetLinearClampSampler());
		m_DeferredTable->AddUniformBufferBinding(6, VK_SHADER_STAGE_FRAGMENT_BIT, m_DeferredUniformBuffer, sizeof(SDeferredLightingUniformBuffer));
		m_DeferredTable->CreateBindings(context);

		const VkFormat sceneColorAttachmentFormat = managers->m_ResourceManager->GetResource(EResourceIndices::SceneColor).m_Format;
		const VkFormat depthFormat = depthAttachment.m_Format;

		m_DeferredPipeline = new CPipeline(EPipelineType::GRAPHICS);
		m_DeferredPipeline->SetVertexShader("shaders/deferred.vert.spv");
		m_DeferredPipeline->SetFragmentShader("shaders/deferred.frag.spv");
		m_DeferredPipeline->SetCullingMode(VK_CULL_MODE_FRONT_BIT);
		m_DeferredPipeline->AddColorAttachment(sceneColorAttachmentFormat);
		m_DeferredPipeline->AddDepthAttachment(depthFormat);
		m_DeferredPipeline->CreatePipeline(context, m_DeferredTable->GetDescriptorSetLayout());
	}

	void CLightingNode::UpdateLightBuffers(CGraphicsContext* context, SGraphicsManagers* managers)
	{
		SDeferredLightingUniformBuffer deferredLightingUbo{};
		deferredLightingUbo.m_Lights[0].m_LightColor     = glm::vec3(1.0f, 1.0f, 1.0f);
		deferredLightingUbo.m_Lights[0].m_LightPosition  = glm::vec3(0.0f, 1000.0f, 30.0f);
		deferredLightingUbo.m_Lights[0].m_LightRadius    = g_LightRadius;
		deferredLightingUbo.m_Lights[0].m_LightIntensity = 100.0f;
		deferredLightingUbo.m_Lights[0].m_LightMatrix    = CShadowNode::GetLightMatrix();
		deferredLightingUbo.m_ViewPos                    = managers->m_InputManager->GetCamera()->GetPosition();
		deferredLightingUbo.m_Pad1                       = 0.0f;

		void* data;
		vkMapMemory(context->GetLogicalDevice(), m_DeferredLightBufferMemory, 0, sizeof(SDeferredLightingUniformBuffer), 0, &data);
		memcpy(data, &deferredLightingUbo, sizeof(SDeferredLightingUniformBuffer));
		vkUnmapMemory(context->GetLogicalDevice(), m_DeferredLightBufferMemory);
	}

	void CLightingNode::Draw(CGraphicsContext* context, SGraphicsManagers* managers, VkCommandBuffer commandBuffer)
	{
		UpdateLightBuffers(context, managers);

		CResourceManager* resourceManager = managers->m_ResourceManager;
		resourceManager->TransitionResource(commandBuffer, EResourceIndices::Positions,          VK_ATTACHMENT_LOAD_OP_LOAD, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		resourceManager->TransitionResource(commandBuffer, EResourceIndices::Normals,            VK_ATTACHMENT_LOAD_OP_LOAD, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		resourceManager->TransitionResource(commandBuffer, EResourceIndices::Albedo,             VK_ATTACHMENT_LOAD_OP_LOAD, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		resourceManager->TransitionResource(commandBuffer, EResourceIndices::Depth,              VK_ATTACHMENT_LOAD_OP_LOAD, VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL);
		resourceManager->TransitionResource(commandBuffer, EResourceIndices::ShadowMap,          VK_ATTACHMENT_LOAD_OP_LOAD, VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL);
		resourceManager->TransitionResource(commandBuffer, EResourceIndices::AtmosphericsSkyBox, VK_ATTACHMENT_LOAD_OP_LOAD, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		SRenderResource sceneColorAttachment = resourceManager->TransitionResource(commandBuffer, EResourceIndices::SceneColor, VK_ATTACHMENT_LOAD_OP_LOAD, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

		std::vector<SRenderResource> sceneColorAttachments = { sceneColorAttachment };
		BeginRendering("Deferred Lighting", context, commandBuffer, sceneColorAttachments);

		m_DeferredTable->BindTable(context, commandBuffer, m_DeferredPipeline->GetPipelineLayout());
		m_DeferredPipeline->BindPipeline(commandBuffer);
		
		// Draw single triangle covering entire screen. See deferred.vert
		vkCmdDraw(commandBuffer, 3, 1, 0, 0);

		EndRendering(context, commandBuffer);
	}

	void CLightingNode::Cleanup(CGraphicsContext* context)
	{
		vkDestroyBuffer(context->GetLogicalDevice(), m_DeferredUniformBuffer, nullptr);
		vkFreeMemory(context->GetLogicalDevice(), m_DeferredLightBufferMemory, nullptr);

		m_DeferredTable->Cleanup(context);
		m_DeferredPipeline->Cleanup(context);

		delete m_DeferredTable;
		delete m_DeferredPipeline;
	}
}