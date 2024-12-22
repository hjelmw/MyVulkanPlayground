#include "ShadowPass.hpp"
#include "GeometryPass.hpp"
#include "../ModelManager.hpp"
#include "../InputManager.hpp"

#include "imgui.h"

#define SHADOWMAP_RESOLUTION 1024

namespace NVulkanEngine
{
	glm::mat4 CShadowPass::s_LightMatrix = glm::identity<glm::mat4>();

	struct SShadowUniformBuffer
	{
		glm::mat4 m_ModelMatrix;
		glm::mat4 m_ViewMatrix;
		glm::mat4 m_ProjectionMatrix;
	};

	std::vector<VkDescriptorSetLayoutBinding> GetShadowBindings()
	{
		std::vector<VkDescriptorSetLayoutBinding> attributeDescriptions(1);

		// 0: Vertex shader uniform buffer
		attributeDescriptions[0].binding = 0;
		attributeDescriptions[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		attributeDescriptions[0].descriptorCount = 1;
		attributeDescriptions[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
		attributeDescriptions[0].pImmutableSamplers = nullptr;

		return attributeDescriptions;
	}

	void CShadowPass::InitPass(CGraphicsContext* context)
	{
		s_ShadowAttachment = CreateRenderAttachment(
			context,
			VK_FORMAT_D32_SFLOAT,
			VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
			VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
			(uint32_t)SHADOWMAP_RESOLUTION,
			(uint32_t)SHADOWMAP_RESOLUTION);

		const std::vector<VkDescriptorSetLayoutBinding>      descriptorSetLayoutBindings = GetShadowBindings();
		const VkVertexInputBindingDescription                vertexBindingDescription    = SVertex::GetVertexBindingDescription();
		const std::vector<VkVertexInputAttributeDescription> vertexAttributeDescriptions = SVertex::GetShadowVertexInputAttributes();

		const std::vector<VkFormat> colorAttachmentFormats = {};
		const VkFormat depthFormat = s_ShadowAttachment.m_Format;

		m_ShadowPipeline = new CPipeline(EGraphicsPipeline);
		m_ShadowPipeline->SetVertexShader("shaders/shadow.vert.spv");
		m_ShadowPipeline->SetFragmentShader("shaders/shadow.frag.spv");
		m_ShadowPipeline->SetCullingMode(VK_CULL_MODE_BACK_BIT);
		m_ShadowPipeline->CreatePipeline(
			context,
			CDrawPass::m_PipelineLayout,
			CDrawPass::m_DescriptorSetLayout,
			descriptorSetLayoutBindings,
			vertexBindingDescription,
			vertexAttributeDescriptions,
			{},
			depthFormat);

		CModelManager* modelManager = CModelManager::GetInstance();

		/* Allocate 2 sets per frame in flight consisting of a single uniform buffer and combined image sampler descriptor */
		AllocateDescriptorPool(context, modelManager->GetNumModels() * 2, 0, modelManager->GetNumModels() * 2);

		m_DescriptorSetsShadow.resize(modelManager->GetNumModels());

		for (uint32_t i = 0; i < modelManager->GetNumModels(); i++)
		{
			CModel* model = modelManager->GetModel(i);

			m_DescriptorSetsShadow[i].m_DescriptorSets = AllocateDescriptorSets(context, CDrawPass::m_DescriptorPool, CDrawPass::m_DescriptorSetLayout, g_MaxFramesInFlight);
			model->CreateShadowMemoryBuffer(context, (VkDeviceSize)sizeof(SShadowUniformBuffer));

			VkDescriptorBufferInfo descriptorUniform = CreateDescriptorBufferInfo(model->GetShadowMemoryBuffer().m_Buffer, sizeof(SShadowUniformBuffer));

			std::vector<VkWriteDescriptorSet> writeDescriptors=
			{
				CreateWriteDescriptorBuffer(context, m_DescriptorSetsShadow[i].m_DescriptorSets.data(), VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, &descriptorUniform),
			};

			UpdateDescriptorSets(context, m_DescriptorSetsShadow[i].m_DescriptorSets, writeDescriptors);
		}
	}

	void CShadowPass::UpdateShadowBuffers(CGraphicsContext* context)
	{
		CModelManager* modelManager = CModelManager::GetInstance();

		for (uint32_t i = 0; i < modelManager->GetNumModels(); i++)
		{
			CModel* model = modelManager->GetModel(i);

			glm::vec3 lightPosition  = CGeometryPass::GetSphereMatrix()[3];
			glm::vec3 lightDirection = normalize(-lightPosition);

			static float left   = 0.0f;
			static float right  = 0.0f;
			static float bottom = 0.0f;
			static float top    = 0.0f;
			static float near   = 0.0f;
			static float far    = 0.0f;

			//ImGui::Begin("Shadow Pass");
			//ImGui::SliderFloat("Left",   &left,   -1000.0f, 1000.0f);
			//ImGui::SliderFloat("Right",  &right,  -1000.0f, 1000.0f);
			//ImGui::SliderFloat("Bottom", &bottom, -1000.0f, 1000.0f);
			//ImGui::SliderFloat("Top",    &top,    -1000.0f, 1000.0f);
			//ImGui::SliderFloat("Near",   &near,   -1000.0f, 1000.0f);
			//ImGui::SliderFloat("Far",    &far,    -1000.0f, 1000.0f);
			//ImGui::End();

			glm::mat4 perspectiveMatrix = glm::ortho(left, right, bottom, top, near, far);

			//orthoMatrix[1][1] *= -1; // Stupid vulkan requirement

			glm::mat4 lookAtMatrix = glm::lookAt(
				lightPosition,
				lightDirection,
				glm::vec3(0.0f, 1.0f, 0.0f)
			);	

			s_LightMatrix = perspectiveMatrix * lookAtMatrix;

			glm::mat4 modelMatrix = model->GetTransform();
			
			SShadowUniformBuffer uboShadow{};
			uboShadow.m_ViewMatrix       = lookAtMatrix;
			uboShadow.m_ProjectionMatrix = perspectiveMatrix;
			uboShadow.m_ModelMatrix      = modelMatrix;

			void* data;
			vkMapMemory(context->GetLogicalDevice(), model->GetShadowMemoryBuffer().m_Memory, 0, sizeof(SShadowUniformBuffer), 0, &data);
			memcpy(data, &uboShadow, sizeof(uboShadow));
			vkUnmapMemory(context->GetLogicalDevice(), model->GetShadowMemoryBuffer().m_Memory);
		}
	}

	void CShadowPass::Draw(CGraphicsContext* context, VkCommandBuffer commandBuffer)
	{
		if (true)
			return;

		BeginRendering(context, commandBuffer, { s_ShadowAttachment });
		UpdateShadowBuffers(context);

		m_ShadowPipeline->Bind(commandBuffer);

		CModelManager* modelManager = CModelManager::GetInstance();
		for (uint32_t i = 0; i < modelManager->GetNumModels(); i++)
		{
			CModel* model = modelManager->GetModel(i);

			VkDescriptorSet shadowDescriptor = m_DescriptorSetsShadow[i].m_DescriptorSets[context->GetFrameIndex() % 2];
			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_PipelineLayout, 0, 1, &shadowDescriptor, 0, nullptr);

			model->Bind(commandBuffer);
			for (uint32_t j = 0; j < model->GetNumMeshes(); j++)
			{
				SMesh modelMesh = model->GetMesh(j);
				vkCmdDrawIndexed(commandBuffer, modelMesh.m_NumVertices, 1, modelMesh.m_StartIndex, 0, 0);
			}
		}

		EndRendering(commandBuffer);
	}

	void CShadowPass::CleanupPass(CGraphicsContext* context)
	{

	}
};