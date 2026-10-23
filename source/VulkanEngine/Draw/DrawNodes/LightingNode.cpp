#include "LightingNode.hpp"
#include "Utils/Pipeline.hpp"
#include "Utils/BindingTable.hpp"
#include "ShadowNode.hpp"

#include <imgui.h>

static float g_LightRadius = 1500.0f;

// Sun is the main light. Direction comes from the shadow node so it matches the shadows and the sky
static float     g_SunIntensity     = 1.0f;
static glm::vec3 g_SunColor         = glm::vec3(1.0f, 1.0f, 1.0f);
static float     g_AmbientIntensity = 0.15f;

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
		float     m_ShadowDepthBias;

		glm::vec3 m_SunDirection; // Towards the sun
		float     m_SunIntensity;
		glm::vec3 m_SunColor;
		float     m_AmbientIntensity;
	};

	void CLightingNode::Init(SRuntimeContext* context, SGraphicsManagers* managers)
	{
		const SRenderResource positionsAttachment    = managers->m_ResourceManager->Get<NRenderResources::SGBufferPositions>();
		const SRenderResource normalsAttachment      = managers->m_ResourceManager->Get<NRenderResources::SGBufferNormals>();
		const SRenderResource albedoAttachment       = managers->m_ResourceManager->Get<NRenderResources::SGBufferAlbedo>();
		const SRenderResource depthAttachment        = managers->m_ResourceManager->Get<NRenderResources::SGBufferDepth>();
		const SRenderResource shadowMapAttachment    = managers->m_ResourceManager->Get<NRenderResources::SSunShadowMap>();
		const SRenderResource atmosphericsAttachment = managers->m_ResourceManager->Get<NRenderResources::SAtmosphericsSkyBox>();

		m_DeferredUniformBuffer = CreateUniformBuffer(context, m_DeferredLightBufferMemory, sizeof(SDeferredLightingUniformBuffer));

		m_DeferredTable = new CBindingTable();
		m_DeferredTable->AddSampledImageBinding(0,  VK_SHADER_STAGE_FRAGMENT_BIT, positionsAttachment.m_ImageView,    positionsAttachment.m_Format,    context->m_LinearClampSampler);
		m_DeferredTable->AddSampledImageBinding(1,  VK_SHADER_STAGE_FRAGMENT_BIT, normalsAttachment.m_ImageView,      normalsAttachment.m_Format,      context->m_LinearClampSampler);
		m_DeferredTable->AddSampledImageBinding(2,  VK_SHADER_STAGE_FRAGMENT_BIT, albedoAttachment.m_ImageView,       albedoAttachment.m_Format,       context->m_LinearClampSampler);
		m_DeferredTable->AddSampledImageBinding(3,  VK_SHADER_STAGE_FRAGMENT_BIT, depthAttachment.m_ImageView,        depthAttachment.m_Format,        context->m_LinearClampSampler);
		m_DeferredTable->AddSampledImageBinding(4,  VK_SHADER_STAGE_FRAGMENT_BIT, shadowMapAttachment.m_ImageView,    shadowMapAttachment.m_Format,    context->m_LinearClampSampler);
		m_DeferredTable->AddSampledImageBinding(5,  VK_SHADER_STAGE_FRAGMENT_BIT, atmosphericsAttachment.m_ImageView, atmosphericsAttachment.m_Format, context->m_LinearClampSampler);
		m_DeferredTable->AddUniformBufferBinding(6, VK_SHADER_STAGE_FRAGMENT_BIT, m_DeferredUniformBuffer, sizeof(SDeferredLightingUniformBuffer));
		m_DeferredTable->CreateBindings(context);

		const VkFormat sceneColorAttachmentFormat = managers->m_ResourceManager->Get<NRenderResources::SSceneColor>().m_Format;
		const VkFormat depthFormat = depthAttachment.m_Format;

		m_DeferredPipeline = new CPipeline(EPipelineType::GRAPHICS);
		m_DeferredPipeline->SetVertexShader("shaders/deferred.vert.spv");
		m_DeferredPipeline->SetFragmentShader("shaders/deferred.frag.spv");
		m_DeferredPipeline->SetCullingMode(VK_CULL_MODE_FRONT_BIT);
		m_DeferredPipeline->AddColorAttachment(sceneColorAttachmentFormat);
		m_DeferredPipeline->AddDepthAttachment(depthFormat);
		m_DeferredPipeline->CreatePipeline(context, m_DeferredTable->GetDescriptorSetLayout());
	}

	void CLightingNode::UpdateBeforeDraw(VkDevice logicalDevice, SGraphicsManagers* managers)
	{
		ImGui::Begin("Lighting");
		ImGui::SliderFloat("Sun Intensity", &g_SunIntensity, 0.0f, 5.0f);
		ImGui::ColorEdit3("Sun Color", &g_SunColor.x);
		ImGui::SliderFloat("Ambient", &g_AmbientIntensity, 0.0f, 1.0f);
		ImGui::End();

		SDeferredLightingUniformBuffer deferredLightingUbo{};
		deferredLightingUbo.m_Lights[0].m_LightColor     = glm::vec3(1.0f, 1.0f, 1.0f);
		deferredLightingUbo.m_Lights[0].m_LightPosition  = glm::vec3(0.0f, 1000.0f, 30.0f);
		deferredLightingUbo.m_Lights[0].m_LightRadius    = g_LightRadius;
		deferredLightingUbo.m_Lights[0].m_LightIntensity = 100.0f;
		deferredLightingUbo.m_Lights[0].m_LightMatrix    = CShadowNode::GetLightMatrix();
		deferredLightingUbo.m_ViewPos                    = managers->m_InputManager->GetCamera()->GetPosition();
		deferredLightingUbo.m_ShadowDepthBias            = CShadowNode::GetShadowDepthBias();
		deferredLightingUbo.m_SunDirection               = CShadowNode::GetSunlightDirection();
		deferredLightingUbo.m_SunIntensity               = g_SunIntensity;
		deferredLightingUbo.m_SunColor                   = g_SunColor;
		deferredLightingUbo.m_AmbientIntensity           = g_AmbientIntensity;

		void* data;
		vkMapMemory(logicalDevice, m_DeferredLightBufferMemory, 0, sizeof(SDeferredLightingUniformBuffer), 0, &data);
		memcpy(data, &deferredLightingUbo, sizeof(SDeferredLightingUniformBuffer));
		vkUnmapMemory(logicalDevice, m_DeferredLightBufferMemory);
	}

	void CLightingNode::Draw(SRuntimeContext* context, SGraphicsManagers* managers, VkCommandBuffer commandBuffer)
	{
		CResourceManager* resourceManager = managers->m_ResourceManager;
		resourceManager->TransitionResource<NRenderResources::SGBufferPositions>(commandBuffer, VK_ATTACHMENT_LOAD_OP_LOAD, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		resourceManager->TransitionResource<NRenderResources::SGBufferNormals>(commandBuffer, VK_ATTACHMENT_LOAD_OP_LOAD, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		resourceManager->TransitionResource<NRenderResources::SGBufferAlbedo>(commandBuffer, VK_ATTACHMENT_LOAD_OP_LOAD, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		resourceManager->TransitionResource<NRenderResources::SGBufferDepth>(commandBuffer, VK_ATTACHMENT_LOAD_OP_LOAD, VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL);
		resourceManager->TransitionResource<NRenderResources::SSunShadowMap>(commandBuffer, VK_ATTACHMENT_LOAD_OP_LOAD, VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL);
		resourceManager->TransitionResource<NRenderResources::SAtmosphericsSkyBox>(commandBuffer, VK_ATTACHMENT_LOAD_OP_LOAD, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		SRenderResource sceneColorAttachment = resourceManager->TransitionResource<NRenderResources::SSceneColor>(commandBuffer, VK_ATTACHMENT_LOAD_OP_LOAD, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

		std::vector<SRenderResource> sceneColorAttachments = { sceneColorAttachment };
		BeginRendering("Deferred Lighting", context, commandBuffer, sceneColorAttachments);

		m_DeferredTable->BindTable(context, commandBuffer, m_DeferredPipeline->GetPipelineLayout());
		m_DeferredPipeline->BindPipeline(commandBuffer);
		
		// Draw single triangle covering entire screen. See deferred.vert
		vkCmdDraw(commandBuffer, 3, 1, 0, 0);

		EndRendering(context, commandBuffer);
	}

	void CLightingNode::Cleanup(SRuntimeContext* context)
	{
		vkDestroyBuffer(context->m_VulkanDevice, m_DeferredUniformBuffer, nullptr);
		vkFreeMemory(context->m_VulkanDevice, m_DeferredLightBufferMemory, nullptr);

		m_DeferredTable->Cleanup(context);
		m_DeferredPipeline->Cleanup(context);

		delete m_DeferredTable;
		delete m_DeferredPipeline;
	}
}