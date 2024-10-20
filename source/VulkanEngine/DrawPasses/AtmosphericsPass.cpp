#include "AtmosphericsPass.hpp"
#include "GeometryPass.hpp"
#include "LightingPass.hpp"

#include "../InputManager.hpp"
#include <imgui.h>

static glm::vec3 g_Wavelengths = glm::vec3(700.0f, 530.0f, 440.0f);

static float g_PlanetRadius = 6000;
static float g_AtmosphereScale = 0.5f;
static float g_DensityFallof = 1.0f;
static float g_ScatteringStrength = 1.0f;
static int   g_NumInscatteringPoints = 16;
static int   g_NumOpticalDepthPoints = 8;

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
		glm::vec3 m_PlanetCameraPosition = glm::vec3(0.0f, 0.0f, 0.0f);
		float m_CameraNear = 0.1f;
		//
		glm::vec3 m_PlanetCenter = glm::vec3(0.0f, 0.0f, 0.0f);
		float m_CameraFar = 10000.0f;
		//
		glm::vec3 m_PlanetToSunDir = glm::vec3(0.0f, 0.0f, 0.0f);
		uint32_t m_NumInScatteringPoints = (uint32_t)g_NumInscatteringPoints;
		//
		uint32_t m_NumOpticalDepthPoints = (uint32_t)g_NumOpticalDepthPoints;
		float m_PlanetRadius = g_PlanetRadius;
		float m_AtmosphereRadius = (1.0f + g_AtmosphereScale) * g_PlanetRadius;
		float m_DensityFallof = g_DensityFallof;
		//
		glm::vec3 m_ScatteringCoefficients = glm::vec3(0.0f, 0.0f, 0.0f);
		float m_Pad0 = 0.0f;
	};

	std::vector<VkDescriptorSetLayoutBinding> GetAtmosphericsBindings()
	{
		std::vector<VkDescriptorSetLayoutBinding> attributeDescriptions(3);

		// 0: Fragment shader scene color
		attributeDescriptions[0].binding            = 0;
		attributeDescriptions[0].descriptorType     = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		attributeDescriptions[0].descriptorCount    = 1;
		attributeDescriptions[0].stageFlags         = VK_SHADER_STAGE_FRAGMENT_BIT;
		attributeDescriptions[0].pImmutableSamplers = nullptr;

		// 1: Fragment shader depth
		attributeDescriptions[1].binding            = 1;
		attributeDescriptions[1].descriptorType     = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		attributeDescriptions[1].descriptorCount    = 1;
		attributeDescriptions[1].stageFlags         = VK_SHADER_STAGE_FRAGMENT_BIT;
		attributeDescriptions[1].pImmutableSamplers = nullptr;

		// 2: Fragment shader uniform buffer
		attributeDescriptions[2].binding            = 2;
		attributeDescriptions[2].descriptorType     = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		attributeDescriptions[2].descriptorCount    = 1;
		attributeDescriptions[2].stageFlags         = VK_SHADER_STAGE_FRAGMENT_BIT;
		attributeDescriptions[2].pImmutableSamplers = nullptr;

		return attributeDescriptions;
	}

	void CAtmosphericsPass::InitPass(CGraphicsContext* context)
	{
		const std::vector<VkDescriptorSetLayoutBinding>      descriptorSetLayoutBindings = GetAtmosphericsBindings();
		const VkVertexInputBindingDescription                vertexBindingDescription = {};
		const std::vector<VkVertexInputAttributeDescription> vertexAttributeDescriptions = {};

		const std::vector<VkFormat> colorAttachmentFormats = { CSwapchain::GetInstance()->GetSwapchainFormat() };
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
			colorAttachmentFormats,
			depthFormat);

		m_AtmosphericsBuffer = CreateBuffer(
			context,
			m_AtmosphericsBufferMemory,
			sizeof(SAtmosphericsFragmentConstants),
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

		AllocateDescriptorPool(context, g_MaxFramesInFlight, g_MaxFramesInFlight * 2, g_MaxFramesInFlight * 1);

		m_AtmosphericsSampler = CreateSampler(context, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, VK_SAMPLER_MIPMAP_MODE_LINEAR, VK_FILTER_NEAREST, VK_FILTER_NEAREST, 0.0f, 0.0f, 1.0f);

		VkDescriptorImageInfo  descriptorSceneColor = CreateDescriptorImageInfo(m_AtmosphericsSampler, CLightingPass::GetSceneColorAttachment().m_ImageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		VkDescriptorImageInfo  descriptorDepth      = CreateDescriptorImageInfo(m_AtmosphericsSampler, CGeometryPass::GetGBufferAttachment(ERenderAttachments::Depth).m_ImageView, VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL);
		VkDescriptorBufferInfo descriptorUniform    = CreateDescriptorBufferInfo(m_AtmosphericsBuffer, sizeof(SAtmosphericsFragmentConstants));

		m_DescriptorSetsAtmospherics = AllocateDescriptorSets(context, CDrawPass::m_DescriptorPool, CDrawPass::m_DescriptorSetLayout, g_MaxFramesInFlight);

		const std::vector<VkWriteDescriptorSet> writeDescriptorSets =
		{
			CreateWriteDescriptorImage(context,  m_DescriptorSetsAtmospherics.data(), VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,  0, &descriptorSceneColor),
			CreateWriteDescriptorImage(context,  m_DescriptorSetsAtmospherics.data(), VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,  1, &descriptorDepth),

			CreateWriteDescriptorBuffer(context, m_DescriptorSetsAtmospherics.data(), VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2, &descriptorUniform)
		};

		UpdateDescriptorSets(context, m_DescriptorSetsAtmospherics, writeDescriptorSets);
	}

	void CAtmosphericsPass::CleanupPass(CGraphicsContext* context)
	{

	}

	void CAtmosphericsPass::UpdateAtmosphericsBuffer(CGraphicsContext* context)
	{
		CCamera* camera = CInputManager::GetInstance()->GetCamera();
		float cameraNear = camera->GetNear();
		float cameraFar = camera->GetFar();

		glm::vec3 planetCameraPos = glm::vec3(0.0f, camera->GetPosition().y, 0.0f);
		glm::vec3 planetCenter    = glm::vec3(0.0f, -g_PlanetRadius, 0.0f);
		glm::vec3 sunDirection    = glm::vec3(0.0f, -1, 0.0f);
		glm::vec3 planetToSunDir  = sunDirection;//glm::normalize(sunDirection - planetCenter);

		float scatterRed   = 1.0f;//powf(400.0f / g_Wavelengths.x, 4.0f) * g_ScatteringStrength;
		float scatterGreen = 1.0f;//powf(400.0f / g_Wavelengths.y, 4.0f) * g_ScatteringStrength;
		float scatterBlue  = 1.0f;//powf(400.0f / g_Wavelengths.z, 4.0f) * g_ScatteringStrength;
		glm::vec3 scatteringCoefficients = glm::vec3(scatterRed, scatterGreen, scatterBlue);

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
		atmosphericsUbo.m_DensityFallof          = g_DensityFallof;
		atmosphericsUbo.m_ScatteringCoefficients = scatteringCoefficients;

		void* data;
		vkMapMemory(context->GetLogicalDevice(), m_AtmosphericsBufferMemory, 0, sizeof(SAtmosphericsFragmentConstants), 0, &data);
		memcpy(data, &atmosphericsUbo, sizeof(SAtmosphericsFragmentConstants));
		vkUnmapMemory(context->GetLogicalDevice(), m_AtmosphericsBufferMemory);
	}


	void CAtmosphericsPass::Draw(CGraphicsContext* context, VkCommandBuffer commandBuffer)
	{
		ImGui::Begin("Atmospherics Pass");
		ImGui::SliderFloat("Atmosphere Radius", &g_AtmosphereScale, 0.0f, 1.0f);
		ImGui::SliderFloat("Planet Radius", &g_PlanetRadius, 0.0f, 6000.0f);
		ImGui::SliderInt("Num Inscattering Points", &g_NumInscatteringPoints, 0, 128);
		ImGui::SliderInt("Num Optical Depth Points", &g_NumOpticalDepthPoints, 0, 64);
		ImGui::SliderFloat("Scattering Strength", &g_ScatteringStrength, 0.0f, 10.0f);
		ImGui::SliderFloat("Density Fallof", &g_DensityFallof, -1.0f, 100.0f);
		ImGui::End();

		UpdateAtmosphericsBuffer(context);

		SImageAttachment sceneColorAttachment = CLightingPass::GetSceneColorAttachment();
		VkImage  sceneImage  = sceneColorAttachment.m_Image;
		VkFormat sceneFormat = sceneColorAttachment.m_Format;

		SImageAttachment depthAttachment = CGeometryPass::GetGBufferAttachment(ERenderAttachments::Depth);
		VkImage  depthImage  = depthAttachment.m_Image;
		VkFormat depthFormat = depthAttachment.m_Format;

		SImageAttachment swapchainAttachment = CDrawPass::GetSwapchainAttachment(context);
		VkImage  swapchainImage = swapchainAttachment.m_Image;
		VkFormat swapchainFormat = swapchainAttachment.m_Format;

		TransitionImageLayout(commandBuffer, swapchainImage, swapchainFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 1);

		TransitionImageLayout(commandBuffer, sceneImage, sceneFormat, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 1);
		TransitionImageLayout(commandBuffer, depthImage, depthFormat, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL, 1);

		std::vector<SImageAttachment> inscatteringAttachments = { swapchainAttachment };
		BeginRendering(context, commandBuffer, inscatteringAttachments);

		m_AtmosphericsPipeline->Bind(commandBuffer);

		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_PipelineLayout, 0, 1, &m_DescriptorSetsAtmospherics[context->GetFrameIndex()], 0, nullptr);

		CCamera* camera = CInputManager::GetInstance()->GetCamera();

		glm::mat4 viewMatrix = camera->GetLookAtMatrix();
		viewMatrix[3] = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);

		SAtmosphericsVertexPushConstants vertexPushConstants{};
		vertexPushConstants.m_ViewMat = viewMatrix;
		vertexPushConstants.m_ProjMat = camera->GetProjectionMatrix();
		vertexPushConstants.m_CameraFar  = 1000.0f;

		vkCmdPushConstants(commandBuffer, CDrawPass::m_PipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, static_cast<uint32_t>(sizeof(SAtmosphericsVertexPushConstants)), &vertexPushConstants);
		vkCmdDraw(commandBuffer, 3, 1, 0, 0);

		EndRendering(commandBuffer);

		TransitionImageLayout(commandBuffer, swapchainImage, swapchainFormat, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, 1);

		TransitionImageLayout(commandBuffer, sceneImage, swapchainFormat, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 1);
		TransitionImageLayout(commandBuffer, depthImage, depthFormat,     VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL,  VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL, 1);

	}
};