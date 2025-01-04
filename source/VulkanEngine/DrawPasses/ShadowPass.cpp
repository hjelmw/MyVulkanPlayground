#include "ShadowPass.hpp"
#include "GeometryPass.hpp"

#include <Managers/InputManager.hpp>
#include <Managers/ModelManager.hpp>

#include <imgui.h>

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

	void CShadowPass::InitPass(CGraphicsContext* context, SGraphicsManagers* managers)
	{
		for (uint32_t i = 0; i < managers->m_Modelmanager->GetNumModels(); i++)
		{
			CModel* model = managers->m_Modelmanager->GetModel(i);
			model->CreateShadowMemoryBuffer(context, (VkDeviceSize)sizeof(SShadowUniformBuffer));
			model->CreateShadowBindingTable(context);
		}

		VkFormat shadowMapFormat = managers->m_AttachmentManager->GetAttachment(EAttachmentIndices::ShadowMap).m_Format;

		// Just get any descriptor set layout since they are the same for all the models atm
		VkDescriptorSetLayout modelDescriptorSetLayout = managers->m_Modelmanager->GetModel(0)->GetModelDescriptorSetLayout();

		m_ShadowPipeline = new CPipeline();
		m_ShadowPipeline->SetVertexShader("shaders/shadow.vert.spv");
		m_ShadowPipeline->SetFragmentShader("shaders/shadow.frag.spv");
		m_ShadowPipeline->SetCullingMode(VK_CULL_MODE_BACK_BIT);
		m_ShadowPipeline->SetVertexInput(sizeof(SModelVertex), VK_VERTEX_INPUT_RATE_VERTEX);
		m_ShadowPipeline->AddVertexAttribute(0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(SModelVertex, m_Position));
		m_ShadowPipeline->AddDepthAttachment(shadowMapFormat);
		m_ShadowPipeline->AddPushConstantSlot(VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(SModelMaterial), 0);
		m_ShadowPipeline->CreatePipeline(context, modelDescriptorSetLayout);
	}

	void CShadowPass::UpdateShadowBuffers(CGraphicsContext* context, SGraphicsManagers* managers)
	{

		for (uint32_t i = 0; i < managers->m_Modelmanager->GetNumModels(); i++)
		{
			CModel* model = managers->m_Modelmanager->GetModel(i);

			glm::vec3 lightPosition  = glm::vec3(0.0f, 1000.0f, 30.0f);
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

			//orthoMatrix[1][1] *= -1; // Some vulkan requirement

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

	void CShadowPass::DrawPass(CGraphicsContext* context, SGraphicsManagers* managers, VkCommandBuffer commandBuffer)
	{
		CAttachmentManager* attachmentManager = managers->m_AttachmentManager;
		SRenderAttachment shadowmapAttachment = attachmentManager->TransitionAttachment(commandBuffer, EAttachmentIndices::ShadowMap, VK_ATTACHMENT_LOAD_OP_LOAD, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

		if (true)
			return;

		BeginRendering(context, commandBuffer, { shadowmapAttachment });
		UpdateShadowBuffers(context, managers);

		m_ShadowPipeline->BindPipeline(commandBuffer);

		for (uint32_t i = 0; i < managers->m_Modelmanager->GetNumModels(); i++)
		{
			CModel* model = managers->m_Modelmanager->GetModel(i);

			model->BindVertexAndIndexBuffers(commandBuffer);
			model->BindShadowTable(context, commandBuffer, m_ShadowPipeline->GetPipelineLayout());
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
		vkDestroyBuffer(context->GetLogicalDevice(), m_ShadowBuffer, nullptr);
		vkFreeMemory(context->GetLogicalDevice(), m_ShadowBufferMemory, nullptr);

		m_ShadowPipeline->Cleanup(context);
		delete m_ShadowPipeline;
	}
};