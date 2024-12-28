#include "GeometryPass.hpp"

#include <imgui.h>

#ifndef M_PI
#define M_PI 3.1415f
#endif

#include <Managers/InputManager.hpp>
#include <Managers/ModelManager.hpp>

static float g_LightPosition[] = { 0.0f, 1000.0f, 30.0f };

namespace NVulkanEngine
{
	glm::mat4 CGeometryPass::s_SphereMatrix = glm::identity<glm::mat4>();

	struct SGeometryUniformBuffer
	{
		glm::mat4 m_ModelMat;
		glm::mat4 m_ViewMat;
		glm::mat4 m_ProjectionMat;
	};

	std::vector<VkDescriptorSetLayoutBinding> GetDescriptorSetLayoutBindings()
	{
		std::vector<VkDescriptorSetLayoutBinding> attributeDescriptions(2);

		// 0: Vertex shader uniform buffer
		attributeDescriptions[0].binding = 0;
		attributeDescriptions[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		attributeDescriptions[0].descriptorCount = 1;
		attributeDescriptions[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
		attributeDescriptions[0].pImmutableSamplers = nullptr;

		// 1: Fragment shader texture sampler
		attributeDescriptions[1].binding = 1;
		attributeDescriptions[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		attributeDescriptions[1].descriptorCount = 1;
		attributeDescriptions[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		attributeDescriptions[1].pImmutableSamplers = nullptr;

		return attributeDescriptions;
	}

	void CGeometryPass::InitPass(CGraphicsContext* context, SGraphicsManagers* managers)
	{
		/* Setup descriptor bindings, vertex binding and vertex attributes */
		const std::vector<VkDescriptorSetLayoutBinding>      descriptorSetLayoutBindings = GetDescriptorSetLayoutBindings();
		const VkVertexInputBindingDescription                vertexBindingDescription    = SVertex::GetVertexBindingDescription();
		const std::vector<VkVertexInputAttributeDescription> vertexAttributeDescriptions = SVertex::GetModelVertexInputAttributes();

		VkFormat positionsFormat = managers->m_AttachmentManager->GetAttachment(EAttachmentIndices::Positions).m_Format;
		VkFormat normalsFormat   = managers->m_AttachmentManager->GetAttachment(EAttachmentIndices::Normals).m_Format;
		VkFormat albedoFormat    = managers->m_AttachmentManager->GetAttachment(EAttachmentIndices::Albedo).m_Format;
		VkFormat depthFormat     = managers->m_AttachmentManager->GetAttachment(EAttachmentIndices::Depth).m_Format;

		const std::vector<VkFormat> colorAttachmentFormats = { positionsFormat, normalsFormat, albedoFormat };

		m_GeometryPipeline = new CPipeline(EGraphicsPipeline);
		m_GeometryPipeline->SetVertexShader("shaders/geometry.vert.spv");
		m_GeometryPipeline->SetFragmentShader("shaders/geometry.frag.spv");
		m_GeometryPipeline->AddPushConstantSlot(VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(SMaterial), 0);
		m_GeometryPipeline->SetCullingMode(VK_CULL_MODE_BACK_BIT);
		m_GeometryPipeline->CreatePipeline(
			context,
			CDrawPass::m_PipelineLayout,
			CDrawPass::m_DescriptorSetLayout,
			descriptorSetLayoutBindings,
			vertexBindingDescription,
			vertexAttributeDescriptions,
			colorAttachmentFormats,
			depthFormat);

		/* Allocate 2 sets per frame in flight consisting of a single uniform buffer and combined image sampler descriptor */
		AllocateDescriptorPool(context, managers->m_Modelmanager->GetNumModels() * 2, managers->m_Modelmanager->GetNumModels() * 2, managers->m_Modelmanager->GetNumModels() * 2);

		for (uint32_t i = 0; i < managers->m_Modelmanager->GetNumModels(); i++)
		{
			CModel* model = managers->m_Modelmanager->GetModel(i);

			SDescriptorSets& modelDescriptorRef = model->GetDescriptorSetsRef();

			modelDescriptorRef.m_DescriptorSets = AllocateDescriptorSets(context, CDrawPass::m_DescriptorPool, CDrawPass::m_DescriptorSetLayout, g_MaxFramesInFlight);
			model->CreateGeometryMemoryBuffer(context, (VkDeviceSize)sizeof(SGeometryUniformBuffer));

			VkDescriptorBufferInfo descriptorUniform = CreateDescriptorBufferInfo(model->GetGeometryMemoryBuffer().m_Buffer, sizeof(SGeometryUniformBuffer));
			VkDescriptorImageInfo descriptorTexture = CreateDescriptorImageInfo(context->GetLinearClampSampler(), model->UsesModelTexture() ? model->GetModelTexture()->GetTextureImageView() : VK_NULL_HANDLE, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

			modelDescriptorRef.m_WriteDescriptors =
			{
				CreateWriteDescriptorBuffer(context, modelDescriptorRef.m_DescriptorSets.data(), VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, &descriptorUniform),
				CreateWriteDescriptorImage(context,  modelDescriptorRef.m_DescriptorSets.data(), VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, &descriptorTexture),
			};

			UpdateDescriptorSets(context, modelDescriptorRef.m_DescriptorSets, modelDescriptorRef.m_WriteDescriptors);
		}
	}

	void CGeometryPass::UpdateGeometryBuffers(CGraphicsContext* context, SGraphicsManagers* managers)
	{
		//m_RotationDegrees = fmod(m_RotationDegrees + 2.0f * context->GetDeltaTime(), 360.0f);

		glm::mat4 sphereMatrix = glm::identity<glm::mat4>();
		sphereMatrix = glm::translate(sphereMatrix, m_LightPosition);
		s_SphereMatrix = sphereMatrix;

		CCamera* camera = managers->m_InputManager->GetCamera();

		for (uint32_t i = 0; i < managers->m_Modelmanager->GetNumModels(); i++)
		{
			CModel* model = managers->m_Modelmanager->GetModel(i);

			SGeometryUniformBuffer uboModel{};
			uboModel.m_ModelMat      = model->GetTransform();
			uboModel.m_ViewMat       = camera->GetLookAtMatrix();
			uboModel.m_ProjectionMat = camera->GetProjectionMatrix();

			void* data;
			vkMapMemory(context->GetLogicalDevice(), model->GetGeometryMemoryBuffer().m_Memory, 0, sizeof(SGeometryUniformBuffer), 0, &data);
			memcpy(data, &uboModel, sizeof(uboModel));
			vkUnmapMemory(context->GetLogicalDevice(), model->GetGeometryMemoryBuffer().m_Memory);
		}
	}

	void CGeometryPass::Draw(CGraphicsContext* context, SGraphicsManagers* managers, VkCommandBuffer commandBuffer)
	{
		CAttachmentManager* attachmentManager = managers->m_AttachmentManager;

		SRenderAttachment positionsAttachment = attachmentManager->TransitionAttachment(commandBuffer, EAttachmentIndices::Positions, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
		SRenderAttachment normalsAttachment   = attachmentManager->TransitionAttachment(commandBuffer, EAttachmentIndices::Normals,   VK_ATTACHMENT_LOAD_OP_CLEAR, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
		SRenderAttachment albedoAttachment    = attachmentManager->TransitionAttachment(commandBuffer, EAttachmentIndices::Albedo,    VK_ATTACHMENT_LOAD_OP_CLEAR, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
		SRenderAttachment depthAttachment     = attachmentManager->TransitionAttachment(commandBuffer, EAttachmentIndices::Depth,     VK_ATTACHMENT_LOAD_OP_CLEAR, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);
		
		std::vector<SRenderAttachment> renderAttachments = { positionsAttachment, normalsAttachment, albedoAttachment, depthAttachment };
		
		BeginRendering(context, commandBuffer, renderAttachments);
		UpdateGeometryBuffers(context, managers);

		m_GeometryPipeline->Bind(commandBuffer);

		for (uint32_t i = 0; i < managers->m_Modelmanager->GetNumModels(); i++)
		{
			CModel* model = managers->m_Modelmanager->GetModel(i);

			VkDescriptorSet modelDescriptor = model->GetDescriptorSetsRef().m_DescriptorSets[context->GetFrameIndex()];
			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_PipelineLayout, 0, 1, &modelDescriptor, 0, nullptr);

			model->Bind(commandBuffer);
			for (uint32_t j = 0; j < model->GetNumMeshes(); j++)
			{
				SMesh modelMesh = model->GetMesh(j);

				SMaterial material          = model->GetMaterial(modelMesh.m_MaterialId);
				material.m_UseAlbedoTexture = model->GetModelTexture() != nullptr;

				vkCmdPushConstants(commandBuffer, CDrawPass::m_PipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, static_cast<uint32_t>(sizeof(SMaterial)), &material);
				vkCmdDrawIndexed(commandBuffer, modelMesh.m_NumVertices, 1, modelMesh.m_StartIndex, 0, 0);

			}
		}

		EndRendering(commandBuffer);
	}

	void CGeometryPass::CleanupPass(CGraphicsContext* context)
	{
		VkDevice device = context->GetLogicalDevice();

		// Descriptor pool and layout (don't need to destroy the sets since they are allocated from the pool)
		vkDestroyDescriptorPool(device, m_DescriptorPool, nullptr);
		vkDestroyDescriptorSetLayout(device, m_DescriptorSetLayout, nullptr);

		// Pipeline and layout
		m_GeometryPipeline->Cleanup(context);
		vkDestroyPipelineLayout(device, m_PipelineLayout, nullptr);
	}
};
