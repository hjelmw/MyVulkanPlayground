#include "GeometryNode.hpp"
#include <imgui.h>

namespace NVulkanEngine
{
	struct SGeometryUniformBuffer
	{
		glm::mat4 m_ModelMat      = glm::identity<glm::mat4>();
		glm::mat4 m_ViewMat       = glm::identity<glm::mat4>();
		glm::mat4 m_ProjectionMat = glm::identity<glm::mat4>();
	};

	void CGeometryNode::Init(CGraphicsContext* context, SGraphicsManagers* managers)
	{
		for (uint32_t i = 0; i < managers->m_Modelmanager->GetNumModels(); i++)
		{
			CModel* model = managers->m_Modelmanager->GetModel(i);
			model->CreateGeometryMemoryBuffer(context, (VkDeviceSize)sizeof(SGeometryUniformBuffer));
			model->CreateGeometryBindingTable(context);
		}

		VkFormat positionsFormat = managers->m_AttachmentManager->GetAttachment(EAttachmentIndices::Positions).m_Format;
		VkFormat normalsFormat   = managers->m_AttachmentManager->GetAttachment(EAttachmentIndices::Normals).m_Format;
		VkFormat albedoFormat    = managers->m_AttachmentManager->GetAttachment(EAttachmentIndices::Albedo).m_Format;
		VkFormat depthFormat     = managers->m_AttachmentManager->GetAttachment(EAttachmentIndices::Depth).m_Format;

		// Just get any descriptor set layout since they are the same for all the models atm
		VkDescriptorSetLayout modelDescriptorSetLayout = managers->m_Modelmanager->GetModel(0)->GetModelDescriptorSetLayout();

		m_GeometryPipeline = new CPipeline();
		m_GeometryPipeline->SetVertexShader("shaders/geometry.vert.spv");
		m_GeometryPipeline->SetFragmentShader("shaders/geometry.frag.spv");
		m_GeometryPipeline->SetCullingMode(VK_CULL_MODE_BACK_BIT);
		m_GeometryPipeline->SetVertexInput(sizeof(SModelVertex), VK_VERTEX_INPUT_RATE_VERTEX);
		m_GeometryPipeline->AddVertexAttribute(0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(SModelVertex, m_Position));
		m_GeometryPipeline->AddVertexAttribute(1, VK_FORMAT_R32G32B32_SFLOAT, offsetof(SModelVertex, m_Color));
		m_GeometryPipeline->AddVertexAttribute(2, VK_FORMAT_R32G32_SFLOAT,    offsetof(SModelVertex, m_TexCoord));
		m_GeometryPipeline->AddVertexAttribute(3, VK_FORMAT_R32G32B32_SFLOAT, offsetof(SModelVertex, m_Normal));
		m_GeometryPipeline->AddVertexAttribute(4, VK_FORMAT_R32G32B32_SFLOAT, offsetof(SModelVertex, m_Tangent));
		m_GeometryPipeline->AddColorAttachment(positionsFormat);
		m_GeometryPipeline->AddColorAttachment(normalsFormat);
		m_GeometryPipeline->AddColorAttachment(albedoFormat);
		m_GeometryPipeline->AddDepthAttachment(depthFormat);
		m_GeometryPipeline->AddPushConstantSlot(VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(SModelMaterial), 0);
		m_GeometryPipeline->CreatePipeline(context, modelDescriptorSetLayout); 
	}

	void CGeometryNode::UpdateGeometryBuffers(CGraphicsContext* context, SGraphicsManagers* managers)
	{
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

	void CGeometryNode::Draw(CGraphicsContext* context, SGraphicsManagers* managers, VkCommandBuffer commandBuffer)
	{
		CAttachmentManager* attachmentManager = managers->m_AttachmentManager;

		SRenderAttachment positionsAttachment = attachmentManager->TransitionAttachment(commandBuffer, EAttachmentIndices::Positions, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
		SRenderAttachment normalsAttachment   = attachmentManager->TransitionAttachment(commandBuffer, EAttachmentIndices::Normals,   VK_ATTACHMENT_LOAD_OP_CLEAR, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
		SRenderAttachment albedoAttachment    = attachmentManager->TransitionAttachment(commandBuffer, EAttachmentIndices::Albedo,    VK_ATTACHMENT_LOAD_OP_CLEAR, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
		SRenderAttachment depthAttachment     = attachmentManager->TransitionAttachment(commandBuffer, EAttachmentIndices::Depth,     VK_ATTACHMENT_LOAD_OP_CLEAR, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);
		
		std::vector<SRenderAttachment> renderAttachments = { positionsAttachment, normalsAttachment, albedoAttachment, depthAttachment };

		BeginRendering("GBuffers", context, commandBuffer, renderAttachments);
		UpdateGeometryBuffers(context, managers);

		m_GeometryPipeline->BindPipeline(commandBuffer);

		for (uint32_t i = 0; i < managers->m_Modelmanager->GetNumModels(); i++)
		{
			CModel* model = managers->m_Modelmanager->GetModel(i);

			managers->m_DebugManager->DrawDebugAABB(model->GetAABB(), glm::vec3(1.0f, 1.0f, 0.0f));

			model->BindVertexAndIndexBuffers(commandBuffer);
			model->BindGeometryTable(context, commandBuffer, m_GeometryPipeline->GetPipelineLayout());
			for (uint32_t j = 0; j < model->GetNumMeshes(); j++)
			{
				SMaterialMesh modelMesh = model->GetMesh(j);
				SModelMaterial material     = model->GetMaterial(modelMesh.m_MaterialId);
				material.m_UseAlbedoTexture = model->GetModelTexture() != nullptr;

				m_GeometryPipeline->PushConstants(commandBuffer, (void*)&material);
				vkCmdDrawIndexed(commandBuffer, modelMesh.m_NumVertices, 1, modelMesh.m_StartIndex, 0, 0);
			}
		}

		EndRendering(context, commandBuffer);
	}

	void CGeometryNode::Cleanup(CGraphicsContext* context)
	{
		m_GeometryPipeline->Cleanup(context);
	}
};
