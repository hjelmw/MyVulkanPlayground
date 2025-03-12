#include "ShadowNode.hpp"

#include <imgui.h>

#include <glm-aabb/AABB.hpp>

#define SHADOWMAP_RESOLUTION 4096

static float g_SunZenithDegrees  = 88.0f;
static float g_SunAzimuthDegrees = 45.0f;

namespace NVulkanEngine
{
	glm::mat4 CShadowNode::s_LightMatrix = glm::identity<glm::mat4>();
	glm::vec3 CShadowNode::s_SunlightDirection = glm::vec3(0.0f, 0.0f, 0.0f);
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

		m_ShadowPipeline = new CPipeline(EPipelineType::GRAPHICS);
		m_ShadowPipeline->SetVertexShader("shaders/shadow.vert.spv");
		m_ShadowPipeline->SetFragmentShader("shaders/shadow.frag.spv");
		m_ShadowPipeline->SetCullingMode(VK_CULL_MODE_NONE);
		m_ShadowPipeline->SetVertexInput(sizeof(SModelVertex), VK_VERTEX_INPUT_RATE_VERTEX);
		m_ShadowPipeline->AddVertexAttribute(0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(SModelVertex, m_Position));
		m_ShadowPipeline->AddDepthAttachment(shadowMapFormat);
		m_ShadowPipeline->AddPushConstantSlot(VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(SModelMaterial), 0);
		m_ShadowPipeline->CreatePipeline(context, modelDescriptorSetLayout);
	}

	glm::mat4 GetZenithAzimuthRotationMatrix(float zenithRadians, float azimuthRadians)
	{
		glm::mat4 rotationZenith = glm::identity<glm::mat4>();
		glm::mat4 rotationAzimuth= glm::identity<glm::mat4>();
		rotationZenith  = glm::rotate(rotationZenith, zenithRadians, glm::vec3(0.0f, 1.0f, 0.0f));
		rotationAzimuth = glm::rotate(rotationAzimuth, azimuthRadians, glm::vec3(1.0f, 0.0f, 0.0f));

		glm::mat4 rotationMatrix = rotationZenith * rotationAzimuth;
		return rotationMatrix;
	}

	void GetSunlightTransformAndDirection(const SGraphicsManagers* managers, glm::vec3& sunlightDirection, glm::mat4& lookatMatrix, glm::mat4& projectionMatrix)
	{
		glm::AABB sceneBounds = managers->m_Modelmanager->GetSceneBounds();

		glm::mat4 sunlightrotationMatrix = glm::identity<glm::mat4>();
		sunlightrotationMatrix = glm::rotate(sunlightrotationMatrix, glm::radians(g_SunAzimuthDegrees), glm::vec3(0.0f, 1.0f, 0.0f));
		sunlightrotationMatrix = glm::rotate(sunlightrotationMatrix, glm::radians(g_SunZenithDegrees), glm::vec3(0.0f, 0.0f, 1.0f));

		glm::vec3 sunlightForwardVector = -(sunlightrotationMatrix * glm::vec4(0.0f, -1.0f, 0.0f, 0.0f));
		glm::vec3 sunlightRightVector = glm::cross(sunlightForwardVector, glm::vec3(0.0f, 1.0f, 0.0f));

		// If vectors are parallel cross product is not defined
		if (glm::length(sunlightRightVector) < 1e-6f)
			sunlightRightVector = glm::vec3(0.1f, 1.0f, 0.0f);

		sunlightRightVector = -glm::normalize(sunlightRightVector);
		glm::vec3 sunlightUpVector = -glm::cross(sunlightForwardVector, -sunlightRightVector);

		float viewMatrixTranslationX    = glm::dot(sceneBounds.getCenter(), sunlightRightVector);
		float viewMatrixTranslationY    = glm::dot(sceneBounds.getCenter(), sunlightUpVector);
		float viewMatrixTranslationZ    = glm::dot(sceneBounds.getCenter(), sunlightForwardVector);
		glm::vec3 viewMatrixTranslation = glm::vec3(viewMatrixTranslationX, viewMatrixTranslationY, viewMatrixTranslationZ);

		// This is just a look at matrix really
		glm::mat4 sunlightViewMatrix = glm::mat4(
			glm::vec4(sunlightRightVector, 0.0f),
			glm::vec4(sunlightUpVector, 0.0f),
			glm::vec4(sunlightForwardVector, 0.0f),
			glm::vec4(viewMatrixTranslation, 1.0f));
		sunlightViewMatrix = glm::inverse(sunlightViewMatrix);
		sceneBounds.transformCorners(sunlightViewMatrix);
		
		// TODO: I think bounds are not tight enough
		glm::mat4 sunlightOrthoMatrix = glm::ortho(
			sceneBounds.getMin().x,
			sceneBounds.getMax().x,
			sceneBounds.getMin().y,
			sceneBounds.getMax().y,
			-sceneBounds.getMax().z,
			-sceneBounds.getMin().z);
		sunlightOrthoMatrix[1][1] *= -1;

		lookatMatrix = sunlightViewMatrix;
		projectionMatrix = sunlightOrthoMatrix;
		sunlightDirection = sunlightForwardVector;
	}

	void CShadowNode::UpdateShadowBuffers(CGraphicsContext* context, SGraphicsManagers* managers)
	{
		//glm::vec3 cameraPosition = managers->m_InputManager->GetCamera()->GetPosition();

		//glm::mat4 sunlightRotationMatrix = GetZenithAzimuthRotationMatrix(glm::radians(g_SunZenithDegrees), glm::radians(g_SunAzimuthDegrees));

		//glm::vec3 sunlightlightPosition  = sunlightRotationMatrix * glm::vec4(0.0f, 1500.0f, 0.0f, 1.0f);
		//glm::vec3 sunlightlightDirection = glm::normalize(-sunlightlightPosition);
		//glm::vec3 sunlightUpDirection    = sunlightRotationMatrix * glm::vec4(0.0f, 0.0f, 1.0f, 1.0f);

		//glm::mat4 sunlightLookAt = glm::lookAt(
		//	sunlightlightPosition,
		//	sunlightlightDirection,
		//	sunlightUpDirection
		//);

		//glm::AABB sceneBounds = managers->m_Modelmanager->GetSceneBounds();

		//glm::mat4 lightOrthoMatrix = glm::ortho(
		//	sceneBounds.getMin().x,
		//	sceneBounds.getMax().x,
		//	sceneBounds.getMin().z,
		//	sceneBounds.getMax().z,
		//	-sceneBounds.getMax().y + sunlightlightPosition.y,
		//	-sceneBounds.getMin().y + sunlightlightPosition.y);
		////glm::mat4 lightOrthoMatrix = glm::ortho(-2000.0f, 2000.0f, -2000.0f, 2000.0f, 1100.0f, 1800.0f);
		//lightOrthoMatrix[1][1] *= -1; // Vulkan requirement

		//glm::mat4 cameraLookAt = managers->m_InputManager->GetCamera()->GetLookAtMatrix();
		//glm::mat4 cameraProjection = managers->m_InputManager->GetCamera()->GetProjectionMatrix();
		//glm::mat4 invCameraViewProjection = glm::inverse(lightOrthoMatrix * sunlightLookAt);

		//// The 8 corners of the camera view frustum in NDC space
		//glm::vec4 corners[8] =
		//{
		//	glm::vec4(-1.0f, -1.0f, 0.0f, 1.0f),
		//	glm::vec4(-1.0f, -1.0f,  1.0f, 1.0f),
		//	glm::vec4( 1.0f, -1.0f, 0.0f, 1.0f),
		//	glm::vec4( 1.0f, -1.0f,  1.0f, 1.0f),
		//	glm::vec4(-1.0f,  1.0f, 0.0f, 1.0f),
		//	glm::vec4(-1.0f,  1.0f,  1.0f, 1.0f), 
		//	glm::vec4( 1.0f,  1.0f, 0.0f, 1.0f),
		//	glm::vec4( 1.0f,  1.0f,  1.0f, 1.0f) 
		//};

		//for (uint32_t i = 0; i < 8; i++)
		//{
		//	corners[i] = invCameraViewProjection * corners[i];
		//	corners[i] /= corners[i].w; // Perspective divide
		//}

		//glm::vec3 debugLineColor = glm::vec3(1.0f, 0.0f, 0.0f);

		//float minX = INFINITY;
		//float minY = INFINITY;
		//float minZ = INFINITY;
		//float maxX = -INFINITY;
		//float maxY = -INFINITY;
		//float maxZ = -INFINITY;

		//// Find the min and max values
		//for (uint32_t i = 0; i < 8; i++)
		//{
		//	if (corners[i].x < minX)
		//		minX = corners[i].x;
		//	if (corners[i].x > maxX)
		//		maxX = corners[i].x;
		//	if (corners[i].y < minY)
		//		minY = corners[i].y;
		//	if (corners[i].y > maxY)
		//		maxY = corners[i].y;
		//	if (corners[i].z < minZ)
		//		minZ = corners[i].z;
		//	if (corners[i].z > maxZ)
		//		maxZ = corners[i].z;
		//}


		//managers->m_DebugManager->DrawDebugAABB(glm::vec3(minX, minY, minZ), glm::vec3(maxX, maxY, maxZ), glm::vec3(1.0f, 0.0f, 0.0f));
		//managers->m_DebugManager->DrawDebugAABB(glm::vec3(minX, minY, -maxZ), glm::vec3(maxX, maxY, -minZ), glm::vec3(1.0f, 0.0f, 0.0f));

		//glm::mat4 lightOrthoMatrix = glm::ortho(minX, maxX, minY, maxY, -maxZ, -minZ);

		static float sunZenithAndAzimuth[2] = { g_SunZenithDegrees, g_SunAzimuthDegrees };

		ImGui::Begin("Shadow Pass");
		ImGui::SliderFloat2("Zenith & Azimuth", sunZenithAndAzimuth, 0.0f, 360.0f);
		g_SunZenithDegrees = sunZenithAndAzimuth[0];
		g_SunAzimuthDegrees = sunZenithAndAzimuth[1];
		ImGui::End();

		glm::mat4 sunlightViewMatrix       = glm::identity<glm::mat4>();
		glm::mat4 sunlightProjectionMatrix = glm::identity<glm::mat4>();
		GetSunlightTransformAndDirection(managers, s_SunlightDirection, sunlightViewMatrix, sunlightProjectionMatrix);

		s_LightMatrix = sunlightProjectionMatrix * sunlightViewMatrix;

		for (uint32_t i = 0; i < managers->m_Modelmanager->GetNumModels(); i++)
		{
			CModel* model = managers->m_Modelmanager->GetModel(i);
			
			SShadowUniformBuffer uboShadow{};
			uboShadow.m_ViewMatrix       = sunlightViewMatrix;
			uboShadow.m_ProjectionMatrix = sunlightProjectionMatrix;
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