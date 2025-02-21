#include "ShadowNode.hpp"

#include <imgui.h>

#define SHADOWMAP_RESOLUTION 2048

namespace NVulkanEngine
{
	glm::mat4 CShadowNode::s_LightMatrix = glm::identity<glm::mat4>();

	struct SShadowUniformBuffer
	{
		glm::mat4 m_ModelMatrix;
		glm::mat4 m_ViewMatrix;
		glm::mat4 m_ProjectionMatrix;
	};

	void CShadowNode::Init(CGraphicsContext* context, SGraphicsManagers* managers)
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
		m_ShadowPipeline->SetCullingMode(VK_CULL_MODE_NONE);
		m_ShadowPipeline->SetVertexInput(sizeof(SModelVertex), VK_VERTEX_INPUT_RATE_VERTEX);
		m_ShadowPipeline->AddVertexAttribute(0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(SModelVertex, m_Position));
		m_ShadowPipeline->AddDepthAttachment(shadowMapFormat);
		m_ShadowPipeline->AddPushConstantSlot(VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(SModelMaterial), 0);
		m_ShadowPipeline->CreatePipeline(context, modelDescriptorSetLayout);
	}

	void CShadowNode::UpdateShadowBuffers(CGraphicsContext* context, SGraphicsManagers* managers)
	{
		glm::vec3 cameraPosition = managers->m_InputManager->GetCamera()->GetPosition();

		//SLightSource sunLight    = managers->m_LightManager->GetSunlight();
		glm::vec3 lightPosition  = glm::vec3(0.0f, 1500.0f, 0.0f);//sunLight.m_Position;
		glm::vec3 lightDirection = glm::vec3(0.0f, -1.0f, 0.0f);//sunLight.m_Direction;

		glm::mat4 lightLookAt = glm::lookAt(
			lightPosition,
			lightDirection,
			glm::vec3(0.0f, 0.0f, 1.0f)
		);

		glm::mat4 cameraLookAt = managers->m_InputManager->GetCamera()->GetLookAtMatrix();
		glm::mat4 cameraProjection = managers->m_InputManager->GetCamera()->GetProjectionMatrix();
		glm::mat4 invCameraViewProjection = glm::inverse(cameraProjection * cameraLookAt);

		// The 8 corners of the camera view frustum in NDC space
		glm::vec4 corners[8] =
		{
			glm::vec4(-1.0f, -1.0f, 1.0f, 1.0f), glm::vec4(-1.0f, -1.0f, -1.0f, 1.0f),
			glm::vec4(-1.0f, 1.0f, 1.0f, 1.0f),  glm::vec4(-1.0f, 1.0f, -1.0f, 1.0f),
			glm::vec4(1.0f, -1.0f, 1.0f, 1.0f),  glm::vec4(1.0f, -1.0f, -1.0f, 1.0f),
			glm::vec4(1.0f, 1.0f, 1.0f, 1.0f),   glm::vec4(1.0f, 1.0f, -1.0f, 1.0f)
		};

		for (uint32_t i = 0; i < 8; i++)
		{
			corners[i] = invCameraViewProjection * corners[i];
			corners[i] /= corners[i].w; // Perspective divide
		}

		float minX = corners[0].x;
		float minY = corners[0].y;
		float minZ = corners[0].z;
		float maxX = corners[0].x;
		float maxY = corners[0].y;
		float maxZ = corners[0].z;

		// Find the min and max values
		for (uint32_t i = 0; i < 8; i++)
		{
			corners[i] = lightLookAt * corners[i];

			if (corners[i].x < minX)
				minX = corners[i].x;
			if (corners[i].x > maxX)
				maxX = corners[i].x;
			if (corners[i].y < minY)
				minY = corners[i].y;
			if (corners[i].y > maxY)
				maxY = corners[i].y;
			if (corners[i].z < minZ)
				minZ = corners[i].z;
			if (corners[i].z > maxZ)
				maxZ = corners[i].z;
		}

		glm::mat4 lightOrthoMatrix = glm::ortho(-2000.0f, 2000.0f, -2000.0f, 2000.0f, 500.0f, 2000.0f);
		//glm::mat4 lightOrthoMatrix = glm::ortho(minX, maxX, minY, maxY, -maxZ, -minZ);
		lightOrthoMatrix[1][1] *= -1; // Vulkan requirement

		ImGui::Begin("Shadow Pass");
		ImGui::Text("Left: %f", minX);
		ImGui::Text("Right: %f", maxX);
		ImGui::Text("Bottom: %f", minY);
		ImGui::Text("Top: %f", maxY);
		ImGui::Text("Near: %f", -maxZ);
		ImGui::Text("Far: %f", -minZ);
		ImGui::End();

		s_LightMatrix = lightOrthoMatrix * lightLookAt;


		for (uint32_t i = 0; i < managers->m_Modelmanager->GetNumModels(); i++)
		{
			CModel* model = managers->m_Modelmanager->GetModel(i);
			
			SShadowUniformBuffer uboShadow{};
			uboShadow.m_ViewMatrix       = lightLookAt;
			uboShadow.m_ProjectionMatrix = lightOrthoMatrix;
			uboShadow.m_ModelMatrix      = model->GetTransform();
			
			void* data;
			vkMapMemory(context->GetLogicalDevice(), model->GetShadowMemoryBuffer().m_Memory, 0, sizeof(SShadowUniformBuffer), 0, &data);
			memcpy(data, &uboShadow, sizeof(uboShadow));
			vkUnmapMemory(context->GetLogicalDevice(), model->GetShadowMemoryBuffer().m_Memory);
		}


	}

	void CShadowNode::Draw(CGraphicsContext* context, SGraphicsManagers* managers, VkCommandBuffer commandBuffer)
	{
		CAttachmentManager* attachmentManager = managers->m_AttachmentManager;
		SRenderAttachment shadowmapAttachment = attachmentManager->TransitionAttachment(commandBuffer, EAttachmentIndices::ShadowMap, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

		VkExtent2D prevRenderResolution = context->GetRenderResolution();
		context->SetRenderResolution(VkExtent2D(SHADOWMAP_RESOLUTION, SHADOWMAP_RESOLUTION));

		BeginRendering("Shadow Map", context, commandBuffer, {shadowmapAttachment});
		UpdateShadowBuffers(context, managers);

		m_ShadowPipeline->BindPipeline(commandBuffer);

		for (uint32_t i = 0; i < managers->m_Modelmanager->GetNumModels(); i++)
		{
			CModel* model = managers->m_Modelmanager->GetModel(i);

			model->BindVertexAndIndexBuffers(commandBuffer);
			model->BindShadowTable(context, commandBuffer, m_ShadowPipeline->GetPipelineLayout());
			for (uint32_t j = 0; j < model->GetNumMeshes(); j++)
			{
				SMaterialMesh modelMesh = model->GetMesh(j);
				vkCmdDrawIndexed(commandBuffer, modelMesh.m_NumVertices, 1, modelMesh.m_StartIndex, 0, 0);
			}
		}

		EndRendering(context, commandBuffer);

		context->SetRenderResolution(prevRenderResolution);
	}

	void CShadowNode::Cleanup(CGraphicsContext* context)
	{
		vkDestroyBuffer(context->GetLogicalDevice(), m_ShadowBuffer, nullptr);
		vkFreeMemory(context->GetLogicalDevice(), m_ShadowBufferMemory, nullptr);

		m_ShadowPipeline->Cleanup(context);
		delete m_ShadowPipeline;
	}
};