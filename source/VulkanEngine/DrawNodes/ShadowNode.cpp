#include "ShadowNode.hpp"

#include <imgui.h>

#include <glm-aabb/AABB.hpp>

#define SHADOWMAP_RESOLUTION 2048

static float nearPlane = 500.0f;
static float farPlane = 2000.0f;

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

		glm::AABB sceneBounds = managers->m_Modelmanager->GetSceneBounds();
		glm::mat4 lightOrthoMatrix = glm::ortho(
			sceneBounds.getMin().x,
			sceneBounds.getMax().x,
			sceneBounds.getMin().z,
			sceneBounds.getMax().z,
			-sceneBounds.getMax().y + lightPosition.y,
			-sceneBounds.getMin().y + lightPosition.y);
		//glm::mat4 lightOrthoMatrix = glm::ortho(-2000.0f, 2000.0f, -2000.0f, 2000.0f, 1100.0f, 1800.0f);
		lightOrthoMatrix[1][1] *= -1; // Vulkan requirement

		glm::mat4 cameraLookAt = managers->m_InputManager->GetCamera()->GetLookAtMatrix();
		glm::mat4 cameraProjection = managers->m_InputManager->GetCamera()->GetProjectionMatrix();
		glm::mat4 invCameraViewProjection = glm::inverse(lightOrthoMatrix * lightLookAt);

		// The 8 corners of the camera view frustum in NDC space
		glm::vec4 corners[8] =
		{
			glm::vec4(-1.0f, -1.0f, 0.0f, 1.0f),
			glm::vec4(-1.0f, -1.0f,  1.0f, 1.0f),
			glm::vec4( 1.0f, -1.0f, 0.0f, 1.0f),
			glm::vec4( 1.0f, -1.0f,  1.0f, 1.0f),
			glm::vec4(-1.0f,  1.0f, 0.0f, 1.0f),
			glm::vec4(-1.0f,  1.0f,  1.0f, 1.0f), 
			glm::vec4( 1.0f,  1.0f, 0.0f, 1.0f),
			glm::vec4( 1.0f,  1.0f,  1.0f, 1.0f) 
		};

		for (uint32_t i = 0; i < 8; i++)
		{
			corners[i] = invCameraViewProjection * corners[i];
			corners[i] /= corners[i].w; // Perspective divide
		}

		glm::vec3 debugLineColor = glm::vec3(1.0f, 0.0f, 0.0f);

		float minX = INFINITY;
		float minY = INFINITY;
		float minZ = INFINITY;
		float maxX = -INFINITY;
		float maxY = -INFINITY;
		float maxZ = -INFINITY;

		// Find the min and max values
		for (uint32_t i = 0; i < 8; i++)
		{
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

		//glm::AABB testAABB = glm::AABB(glm::vec3(-400.0f, -100.0f, -200.0f), glm::vec3(300.0f, 400.0f, 200.0f));
		//managers->m_DebugManager->DrawDebugAABB(testAABB.getMin(), testAABB.getMax(), glm::vec3(0.0f, 1.0f, 0.0f));

		//managers->m_DebugManager->DrawDebugAABB(glm::vec3(minX, minY, minZ), glm::vec3(maxX, maxY, maxZ), glm::vec3(1.0f, 0.0f, 0.0f));
		managers->m_DebugManager->DrawDebugAABB(glm::vec3(minX, minY, -maxZ), glm::vec3(maxX, maxY, -minZ), glm::vec3(1.0f, 0.0f, 0.0f));

		//glm::mat4 lightOrthoMatrix = glm::ortho(minX, maxX, minY, maxY, -maxZ, -minZ);



		ImGui::Begin("Shadow Pass");
		ImGui::SliderFloat("Near Plane", &nearPlane, 0.0f, 10000.0f);
		ImGui::SliderFloat("Far Plane", &farPlane, 0.0f, 10000.0f);

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
		CDebugManager* debugManager = managers->m_DebugManager;

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