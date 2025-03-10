#include "SkyNode.hpp"
#include "ShadowNode.hpp"

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
		float m_CameraFar   = 0.0f;
	};
	struct SAtmosphericsFragmentConstants
	{
		glm::vec3    m_PlanetCameraPosition   = glm::vec3(0.0f, 0.0f, 0.0f);
		glm::float32 m_CameraNear             = 0.0f;
		//
		glm::vec3    m_PlanetCenter           = glm::vec3(0.0f, 0.0f, 0.0f);
		glm::float32 m_CameraFar              = 0.0f;
		//
		glm::vec3     m_PlanetToSunDir        = glm::vec3(0.0f, 0.0f, 0.0f);
		glm::uint32_t m_NumInScatteringPoints = 0;
		//
		glm::uint32_t m_NumOpticalDepthPoints = 0;
		glm::float32  m_PlanetRadius          = 0.0f;
		glm::float32  m_AtmosphereRadius      = 0.0f;
		glm::float32  m_AbsorptionFallof      = 0.0f;
		//
		glm::vec3    m_AbsorptionBeta         = glm::vec3(0.0f, 0.0f, 0.0f);
		glm::float32 m_AbsorptionHeight       = 0.0f;
		//
		glm::vec3    m_RayleighBetaScattering = glm::vec3(0.0f, 0.0f, 0.0f);
		glm::float32 m_RayleighHeight         = 0.0f;
		//
		glm::vec3    m_MieBetaScattering      = glm::vec3(0.0f, 0.0f, 0.0f);
		glm::float32 m_MieHeight              = 0.0f;
		//
		glm::uint32_t m_AllowMieScattering    = 0;
		glm::float32  m_ScatteringIntensity   = 0.0f;
		glm::vec2     m_Pad0                  = glm::vec2(0xdeadbeef, 0xdeadbeef);
	};

	void CSkyNode::Init(CGraphicsContext* context, SGraphicsManagers* managers)
	{
		const SRenderAttachment atmosphericsAttachment  = managers->m_AttachmentManager->GetAttachment(EAttachmentIndices::AtmosphericsSkyBox);
		const SRenderAttachment depthAttachment         = managers->m_AttachmentManager->GetAttachment(EAttachmentIndices::Depth);

		m_AtmosphericsUniformBuffer = CreateUniformBuffer(context, m_AtmosphericsBufferMemory, sizeof(SAtmosphericsFragmentConstants));

		m_AtmosphericsTable = new CBindingTable();
		m_AtmosphericsTable->AddSampledImageBinding(0, VK_SHADER_STAGE_FRAGMENT_BIT, depthAttachment.m_ImageView, depthAttachment.m_Format, context->GetLinearClampSampler());
		m_AtmosphericsTable->AddUniformBufferBinding(1, VK_SHADER_STAGE_FRAGMENT_BIT, m_AtmosphericsUniformBuffer, sizeof(SAtmosphericsFragmentConstants));
		m_AtmosphericsTable->CreateBindings(context);

		m_AtmosphericsPipeline = new CPipeline();
		m_AtmosphericsPipeline->SetVertexShader("shaders/atmospherics.vert.spv");
		m_AtmosphericsPipeline->SetFragmentShader("shaders/atmospherics.frag.spv");
		m_AtmosphericsPipeline->SetCullingMode(VK_CULL_MODE_FRONT_BIT);
		m_AtmosphericsPipeline->AddColorAttachment(atmosphericsAttachment.m_Format);
		m_AtmosphericsPipeline->AddDepthAttachment(depthAttachment.m_Format);
		m_AtmosphericsPipeline->AddPushConstantSlot(VK_SHADER_STAGE_VERTEX_BIT, sizeof(SAtmosphericsVertexPushConstants), 0);
		m_AtmosphericsPipeline->CreatePipeline(context, m_AtmosphericsTable->GetDescriptorSetLayout());
	}

	void CSkyNode::UpdateAtmosphericsConstants(CGraphicsContext* context, SGraphicsManagers* managers)
	{
		CCamera* camera = managers->m_InputManager->GetCamera();
		float cameraNear = camera->GetNear();
		float cameraFar = camera->GetFar();

		glm::vec3 planetCameraPos = glm::vec3(0.0f, g_PlanetRadius + camera->GetPosition().y, 0.0f);
		glm::vec3 planetCenter    = glm::vec3(0.0f, 0.0f, 0.0f);
		glm::vec3 sunDirection    = CShadowNode::GetSunlightDirection();
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

	void CSkyNode::Draw(CGraphicsContext* context, SGraphicsManagers* managers, VkCommandBuffer commandBuffer)
	{
		ImGui::Begin("Atmospherics");
		ImGui::SliderFloat("Atmosphere Radius", &g_AtmosphereScale, 0.0f, 1.0f);
		ImGui::SliderFloat("Planet Radius", &g_PlanetRadius, 0.0f, 6000.0f);
		ImGui::SliderInt("Num Inscattering Points", &g_NumInscatteringPoints, 0, 128);
		ImGui::SliderInt("Num Optical Depth Points", &g_NumOpticalDepthPoints, 0, 64);
		ImGui::SliderFloat("Scattering Intensity", &g_ScatteringIntensity, 0.0f, 1000.0f);
		ImGui::End();

		CCamera* camera = managers->m_InputManager->GetCamera();
		glm::mat4 viewMatrix = camera->GetLookAtMatrix();
		viewMatrix[3] = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);

		SAtmosphericsVertexPushConstants vertexPushConstants{};
		vertexPushConstants.m_ViewMat   = viewMatrix;
		vertexPushConstants.m_ProjMat   = camera->GetProjectionMatrix();
		vertexPushConstants.m_CameraFar = camera->GetFar();

		UpdateAtmosphericsConstants(context, managers);

		CAttachmentManager* attachmentManager = managers->m_AttachmentManager;
		attachmentManager->TransitionAttachment(commandBuffer, EAttachmentIndices::Depth, VK_ATTACHMENT_LOAD_OP_LOAD, VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL);
		SRenderAttachment atmosphericsAttachment = attachmentManager->TransitionAttachment(commandBuffer, EAttachmentIndices::AtmosphericsSkyBox, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

		std::vector<SRenderAttachment> inscatteringAttachments = { atmosphericsAttachment };
		BeginRendering("Skybox", context, commandBuffer, inscatteringAttachments);

		m_AtmosphericsTable->BindTable(context, commandBuffer, m_AtmosphericsPipeline->GetPipelineLayout());
		m_AtmosphericsPipeline->BindPipeline(commandBuffer);
		m_AtmosphericsPipeline->PushConstants(commandBuffer, (void*)&vertexPushConstants);

		vkCmdDraw(commandBuffer, 3, 1, 0, 0);

		EndRendering(context, commandBuffer);
	}

	void CSkyNode::Cleanup(CGraphicsContext* context)
	{
		VkDevice device = context->GetLogicalDevice();

		vkDestroyBuffer(device, m_AtmosphericsUniformBuffer, nullptr);
		vkFreeMemory(device, m_AtmosphericsBufferMemory, nullptr);

		m_AtmosphericsTable->Cleanup(context);
		m_AtmosphericsPipeline->Cleanup(context);

		delete m_AtmosphericsTable;
		delete m_AtmosphericsPipeline;
	}

};