#include "AtmosphericsPass.hpp"

#include <DrawPasses/GeometryPass.hpp>
#include <DrawPasses/LightingPass.hpp>

#include <Managers/InputManager.hpp>

#include <imgui.h>

//
// I took all these values from shadertoy so no idea if they are scientifically accurate
// Source https://www.shadertoy.com/view/wlBXWK
// 

// Atmosphere skybox size
static float     g_PlanetRadius = 6340e3;
static float     g_AtmosphereScale = 0.015f; // Atmosphere radius is x% larger than planet radius
// Scattering coefficients
static constexpr glm::vec3 g_RayleighBetaScattering = glm::vec3(5.5e-6f, 13.0e-6f, 22.4e-6f);
static constexpr glm::vec3 g_MieBetaScattering      = glm::vec3(21e-6, 21e-6, 21e-6);
static constexpr glm::vec3 g_AmbientBeta            = glm::vec3(0.0f, 0.0f, 0.0f);
static constexpr glm::vec3 g_AbsorptionBeta         = glm::vec3(2.04e-5, 4.97e-5, 1.95e-6);

// Height values, i.e up to what height does the scattering no longer have an effect
static constexpr float g_RayleighMaxHeight   = 8e3f;
static constexpr float g_MieMaxHeight        = 1.2e3f;
static constexpr float g_AbsorptionMaxHeight = 30e3f;
static constexpr float g_AbsorptionFallof    = 4e3f;

static float     g_ScatteringIntensity   = 40.0f;
static int       g_NumInscatteringPoints = 16;
static int       g_NumOpticalDepthPoints = 8;

namespace NVulkanEngine
{
	struct SAtmosphericsVertexPushConstants
	{
		glm::mat4 m_ViewMat = glm::identity<glm::mat4>();
		//
		glm::mat4 m_ProjMat = glm::identity<glm::mat4>();
		//
		float m_CameraFar = 0.0f;
	};
	struct SAtmosphericsFragmentConstants
	{
		glm::vec3    m_PlanetCameraPosition = glm::vec3(0.0f, 0.0f, 0.0f);
		glm::float32 m_CameraNear           = 0.0f;
		//
		glm::vec3    m_PlanetCenter = glm::vec3(0.0f, 0.0f, 0.0f);
		glm::float32 m_CameraFar    = 0.0f;
		//
		glm::vec3     m_PlanetToSunDir        = glm::vec3(0.0f, 0.0f, 0.0f);
		glm::uint32_t m_NumInScatteringPoints = 0;
		//
		glm::uint32_t m_NumOpticalDepthPoints = 0;
		glm::float32  m_PlanetRadius          = 0.0f;
		glm::float32  m_AtmosphereRadius      = 0.0f;
		glm::float32  m_AbsorptionFallof      = 0.0f;
		//
		glm::vec3    m_AbsorptionBeta        = glm::vec3(0.0f, 0.0f, 0.0f);
		glm::float32 m_AbsorptionHeight      = 0.0f;
		//
		glm::vec3    m_RayleighBetaScattering = glm::vec3(0.0f, 0.0f, 0.0f);
		glm::float32 m_RayleighHeight         = 0.0f;
		//
		glm::vec3    m_MieBetaScattering = glm::vec3(0.0f, 0.0f, 0.0f);
		glm::float32 m_MieHeight         = 0.0f;
		//
		glm::uint32_t m_AllowMieScattering  = 0;
		glm::float32  m_ScatteringIntensity = 0.0f;
		glm::vec2     m_Pad0                = glm::vec2(0xdeadbeef, 0xdeadbeef);
	};

	std::vector<VkDescriptorSetLayoutBinding> GetAtmosphericsBindings()
	{
		std::vector<VkDescriptorSetLayoutBinding> attributeDescriptions(2);

		// 1: Fragment shader depth
		attributeDescriptions[0].binding            = 0;
		attributeDescriptions[0].descriptorType     = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		attributeDescriptions[0].descriptorCount    = 1;
		attributeDescriptions[0].stageFlags         = VK_SHADER_STAGE_FRAGMENT_BIT;
		attributeDescriptions[0].pImmutableSamplers = nullptr;

		// 2: Fragment shader uniform bufferz
		attributeDescriptions[1].binding            = 1;
		attributeDescriptions[1].descriptorType     = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		attributeDescriptions[1].descriptorCount    = 1;
		attributeDescriptions[1].stageFlags         = VK_SHADER_STAGE_FRAGMENT_BIT;
		attributeDescriptions[1].pImmutableSamplers = nullptr;

		return attributeDescriptions;
	}

	void CAtmosphericsPass::InitPass(CGraphicsContext* context, SGraphicsManagers* managers)
	{
		SRenderAttachment atmosphericsAttachment = managers->m_AttachmentManager->AddAttachment(
			context,
			"Atmospherics SkyBox",
			EAttachmentIndices::AtmosphericsSkyBox,
			context->GetLinearClampSampler(),
			VK_FORMAT_R8G8B8A8_UNORM,
			VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			context->GetRenderResolution().width,
			context->GetRenderResolution().height);

		const std::vector<VkDescriptorSetLayoutBinding>      descriptorSetLayoutBindings = GetAtmosphericsBindings();
		const VkVertexInputBindingDescription                vertexBindingDescription = {};
		const std::vector<VkVertexInputAttributeDescription> vertexAttributeDescriptions = {};

		const std::vector<VkFormat> sceneColor = { atmosphericsAttachment.m_Format };
		const VkFormat depthFormat = VK_FORMAT_UNDEFINED;

		m_AtmosphericsPipeline = new CPipeline(EGraphicsPipeline);
		m_AtmosphericsPipeline->SetVertexShader("shaders/atmospherics.vert.spv");
		m_AtmosphericsPipeline->SetFragmentShader("shaders/atmospherics.frag.spv");
		m_AtmosphericsPipeline->SetCullingMode(VK_CULL_MODE_FRONT_BIT);
		m_AtmosphericsPipeline->AddPushConstantSlot(VK_SHADER_STAGE_VERTEX_BIT, sizeof(SAtmosphericsVertexPushConstants), 0);
		m_AtmosphericsPipeline->CreatePipeline(
			context,
			CDrawPass::m_PipelineLayout,
			CDrawPass::m_DescriptorSetLayout,
			descriptorSetLayoutBindings,
			vertexBindingDescription,
			vertexAttributeDescriptions,
			sceneColor,
			depthFormat);

		m_AtmosphericsBuffer = CreateBuffer(
			context,
			m_AtmosphericsBufferMemory,
			sizeof(SAtmosphericsFragmentConstants),
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

		AllocateDescriptorPool(context, g_MaxFramesInFlight, g_MaxFramesInFlight * 2, g_MaxFramesInFlight * 1);

		SRenderAttachment depthAttachment = managers->m_AttachmentManager->GetAttachment(EAttachmentIndices::Depth);

		VkDescriptorImageInfo  descriptorDepth   = CreateDescriptorImageInfo(context->GetLinearClampSampler(), depthAttachment.m_ImageView, VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL);
		VkDescriptorBufferInfo descriptorUniform = CreateDescriptorBufferInfo(m_AtmosphericsBuffer, sizeof(SAtmosphericsFragmentConstants));

		m_DescriptorSetsAtmospherics = AllocateDescriptorSets(context, CDrawPass::m_DescriptorPool, CDrawPass::m_DescriptorSetLayout, g_MaxFramesInFlight);

		const std::vector<VkWriteDescriptorSet> writeDescriptorSets =
		{
			CreateWriteDescriptorImage(context,  m_DescriptorSetsAtmospherics.data(), VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,  0, &descriptorDepth),

			CreateWriteDescriptorBuffer(context, m_DescriptorSetsAtmospherics.data(), VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, &descriptorUniform)
		};

		UpdateDescriptorSets(context, m_DescriptorSetsAtmospherics, writeDescriptorSets);
	}

	void CAtmosphericsPass::CleanupPass(CGraphicsContext* context)
	{
		VkDevice device = context->GetLogicalDevice();

		vkDestroyBuffer(device, m_AtmosphericsBuffer, nullptr);
		vkFreeMemory(device, m_AtmosphericsBufferMemory, nullptr);

		vkDestroyDescriptorPool(device, m_DescriptorPool, nullptr);
		vkDestroyDescriptorSetLayout(device, m_DescriptorSetLayout, nullptr);

		m_AtmosphericsPipeline->Cleanup(context);
		delete m_AtmosphericsPipeline;
	}

	void CAtmosphericsPass::UpdateAtmosphericsBuffer(CGraphicsContext* context, SGraphicsManagers* managers)
	{
		CCamera* camera = managers->m_InputManager->GetCamera();
		float cameraNear = camera->GetNear();
		float cameraFar = camera->GetFar();

		glm::vec3 planetCameraPos = glm::vec3(0.0f, g_PlanetRadius + camera->GetPosition().y, 0.0f);
		glm::vec3 planetCenter    = glm::vec3(0.0f, 0.0f, 0.0f);
		glm::vec3 sunDirection    = -glm::vec3(0.0f, -1.0f, 0.0f);
		glm::vec3 planetToSunDir  = sunDirection;

		SAtmosphericsFragmentConstants atmosphericsUbo{};
		atmosphericsUbo.m_PlanetToSunDir         = planetToSunDir;
		atmosphericsUbo.m_PlanetCameraPosition   = planetCameraPos;
		atmosphericsUbo.m_AtmosphereRadius		 = (1.0f + g_AtmosphereScale) * g_PlanetRadius;
		atmosphericsUbo.m_PlanetRadius           = g_PlanetRadius;
		atmosphericsUbo.m_PlanetCenter           = planetCenter;
		atmosphericsUbo.m_NumInScatteringPoints  = g_NumInscatteringPoints;
		atmosphericsUbo.m_NumOpticalDepthPoints  = g_NumOpticalDepthPoints;
		atmosphericsUbo.m_CameraNear             = cameraNear;
		atmosphericsUbo.m_CameraFar              = cameraFar;
		atmosphericsUbo.m_AbsorptionFallof       = g_AbsorptionFallof;
		atmosphericsUbo.m_AbsorptionBeta         = g_AbsorptionBeta;
		atmosphericsUbo.m_AbsorptionHeight       = g_AbsorptionMaxHeight;
		atmosphericsUbo.m_RayleighBetaScattering = g_RayleighBetaScattering;
		atmosphericsUbo.m_RayleighHeight         = g_RayleighMaxHeight;
		atmosphericsUbo.m_MieBetaScattering      = g_MieBetaScattering;
		atmosphericsUbo.m_MieHeight              = g_MieMaxHeight;
		atmosphericsUbo.m_AllowMieScattering     = true;
		atmosphericsUbo.m_ScatteringIntensity    = g_ScatteringIntensity;

		void* data;
		vkMapMemory(context->GetLogicalDevice(), m_AtmosphericsBufferMemory, 0, sizeof(SAtmosphericsFragmentConstants), 0, &data);
		memcpy(data, &atmosphericsUbo, sizeof(SAtmosphericsFragmentConstants));
		vkUnmapMemory(context->GetLogicalDevice(), m_AtmosphericsBufferMemory);
	}


	void CAtmosphericsPass::Draw(CGraphicsContext* context, SGraphicsManagers* managers, VkCommandBuffer commandBuffer)
	{

		ImGui::Begin("Atmospherics Pass");
		ImGui::SliderFloat("Atmosphere Radius", &g_AtmosphereScale, 0.0f, 1.0f);
		ImGui::SliderFloat("Planet Radius", &g_PlanetRadius, 0.0f, 6000.0f);
		ImGui::SliderInt("Num Inscattering Points", &g_NumInscatteringPoints, 0, 128);
		ImGui::SliderInt("Num Optical Depth Points", &g_NumOpticalDepthPoints, 0, 64);
		ImGui::SliderFloat("Scattering Intensity", &g_ScatteringIntensity, 0.0f, 1000.0f);
		ImGui::End();

		UpdateAtmosphericsBuffer(context, managers);

		CAttachmentManager* attachmentManager = managers->m_AttachmentManager;
		attachmentManager->TransitionAttachment(commandBuffer, EAttachmentIndices::Depth, VK_ATTACHMENT_LOAD_OP_LOAD, VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL);
		SRenderAttachment atmosphericsAttachment = attachmentManager->TransitionAttachment(commandBuffer, EAttachmentIndices::AtmosphericsSkyBox, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

		std::vector<SRenderAttachment> inscatteringAttachments = { atmosphericsAttachment };
		BeginRendering(context, commandBuffer, inscatteringAttachments);

		m_AtmosphericsPipeline->Bind(commandBuffer);

		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_PipelineLayout, 0, 1, &m_DescriptorSetsAtmospherics[context->GetFrameIndex()], 0, nullptr);

		CCamera* camera = managers->m_InputManager->GetCamera();

		glm::mat4 viewMatrix = camera->GetLookAtMatrix();
		viewMatrix[3] = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);

		SAtmosphericsVertexPushConstants vertexPushConstants{};
		vertexPushConstants.m_ViewMat = viewMatrix;
		vertexPushConstants.m_ProjMat = camera->GetProjectionMatrix();
		vertexPushConstants.m_CameraFar  = 1000.0f;

		vkCmdPushConstants(commandBuffer, CDrawPass::m_PipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, static_cast<uint32_t>(sizeof(SAtmosphericsVertexPushConstants)), &vertexPushConstants);
		vkCmdDraw(commandBuffer, 3, 1, 0, 0);

		EndRendering(commandBuffer);
	}
};