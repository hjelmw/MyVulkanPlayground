#include "ShadowNode.hpp"
#include "TerrainNode.hpp"
#include "Utils/Pipeline.hpp"


#include <imgui.h>

#include <glm-aabb/AABB.hpp>

#define SHADOWMAP_RESOLUTION 4096

static float g_SunZenithDegrees  = 45.0f;
static float g_SunAzimuthDegrees = 45.0f;

// How far behind the occluder (in world units) a receiver must be to count as shadowed
static float g_ShadowBiasWorldUnits = 64.0f;

namespace NVulkanEngine
{
	glm::mat4 CShadowNode::s_LightMatrix = glm::identity<glm::mat4>();
	glm::vec3 CShadowNode::s_SunlightDirection = glm::vec3(0.0f, 0.0f, 0.0f);
	float     CShadowNode::s_ShadowDepthBias = 0.0f;
	struct SShadowUniformBuffer
	{
		glm::mat4 m_ModelMatrix;
		glm::mat4 m_ViewMatrix;
		glm::mat4 m_ProjectionMatrix;
	};

	struct STerrainShadowVertexPushConstants
	{
		glm::mat4 m_ViewProjectionMatrix = glm::identity<glm::mat4>();
	};

	void CShadowNode::Init(CGraphicsContext* context, SGraphicsManagers* managers)
	{
		for (uint32_t i = 0; i < managers->m_Modelmanager->GetNumModels(); i++)
		{
			CModel* model = managers->m_Modelmanager->GetModel(i);
			model->CreateShadowMemoryBuffer(context, (VkDeviceSize)sizeof(SShadowUniformBuffer));
			model->CreateShadowBindingTable(context);
		}

		for (uint32_t i = 0; i < managers->m_Modelmanager->GetNumTerrainModels(); i++)
		{
			CModel* terrainModel = managers->m_Modelmanager->GetTerrainModel(i);
			terrainModel->CreateShadowMemoryBuffer(context, (VkDeviceSize)sizeof(SShadowUniformBuffer));
			terrainModel->CreateShadowBindingTable(context);
		}

		VkFormat shadowMapFormat = managers->m_ResourceManager->GetRenderResource(EResourceIndices::ShadowMap).m_Format;

		// Just get any descriptor set layout since they are the same for all the models atm
		VkDescriptorSetLayout modelDescriptorSetLayout = managers->m_Modelmanager->GetModel(0)->GetModelDescriptorSetLayout();

		m_ShadowPipeline = new CPipeline(EPipelineType::GRAPHICS);
		m_ShadowPipeline->SetDebugName("Shadowmap Models");
		m_ShadowPipeline->SetVertexShader("shaders/shadow.vert.spv");
		m_ShadowPipeline->SetFragmentShader("shaders/shadow.frag.spv");
		m_ShadowPipeline->SetCullingMode(VK_CULL_MODE_BACK_BIT);
		m_ShadowPipeline->SetPrimitiveTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
		m_ShadowPipeline->SetVertexInput(sizeof(SModelVertex), VK_VERTEX_INPUT_RATE_VERTEX);
		m_ShadowPipeline->AddVertexAttribute(0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(SModelVertex, m_Position));
		m_ShadowPipeline->AddDepthAttachment(shadowMapFormat);
		m_ShadowPipeline->AddPushConstantSlot(VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(SModelMaterial), 0);
		m_ShadowPipeline->CreatePipeline(context, modelDescriptorSetLayout);

		m_TerrainShadowPipeline = new CPipeline(EPipelineType::GRAPHICS);
		m_TerrainShadowPipeline->SetDebugName("Shadowmap Terrain");
		m_TerrainShadowPipeline->SetVertexShader("shaders/shadow.vert.spv");
		m_TerrainShadowPipeline->SetFragmentShader("shaders/shadow.frag.spv");
		m_TerrainShadowPipeline->SetCullingMode(VK_CULL_MODE_BACK_BIT);
		m_TerrainShadowPipeline->SetDepthBias(0.0f, 1.5f); // Slope scaled bias against acne from the low sun grazing the heightmap
		m_TerrainShadowPipeline->SetPrimitiveTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP);
		m_TerrainShadowPipeline->SetVertexInput((uint32_t)CTerrainNode::GetVertexStride(), VK_VERTEX_INPUT_RATE_VERTEX);
		m_TerrainShadowPipeline->AddVertexAttribute(0, VK_FORMAT_R32G32B32_SFLOAT, 0);
		m_TerrainShadowPipeline->AddDepthAttachment(shadowMapFormat);
		m_TerrainShadowPipeline->CreatePipeline(context, modelDescriptorSetLayout);
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

	void GetSunlightTransformAndDirection(const SGraphicsManagers* managers, glm::vec3& sunlightDirection, glm::mat4& lookatMatrix, glm::mat4& projectionMatrix, float& depthRange)
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
		depthRange = sceneBounds.getMax().z - sceneBounds.getMin().z;
	}

	void CShadowNode::UpdateBeforeDraw(VkDevice logicalDevice, SGraphicsManagers* managers)
	{
		static float sunZenithAndAzimuth[2] = { g_SunZenithDegrees, g_SunAzimuthDegrees };

		ImGui::Begin("Shadow Pass");
		ImGui::SliderFloat2("Zenith & Azimuth", sunZenithAndAzimuth, 0.0f, 360.0f);
		ImGui::SliderFloat("Bias (world units)", &g_ShadowBiasWorldUnits, 0.0f, 1000.0f);
		g_SunZenithDegrees = sunZenithAndAzimuth[0];
		g_SunAzimuthDegrees = sunZenithAndAzimuth[1];
		ImGui::End();

		glm::mat4 sunlightViewMatrix       = glm::identity<glm::mat4>();
		glm::mat4 sunlightProjectionMatrix = glm::identity<glm::mat4>();
		float     sunlightDepthRange       = 1.0f;
		GetSunlightTransformAndDirection(managers, s_SunlightDirection, sunlightViewMatrix, sunlightProjectionMatrix, sunlightDepthRange);

		s_LightMatrix     = sunlightProjectionMatrix * sunlightViewMatrix;
		s_ShadowDepthBias = g_ShadowBiasWorldUnits / sunlightDepthRange;

		for (uint32_t i = 0; i < managers->m_Modelmanager->GetNumModels(); i++)
		{
			CModel* model = managers->m_Modelmanager->GetModel(i);
			
			SShadowUniformBuffer uboShadow{};
			uboShadow.m_ViewMatrix       = sunlightViewMatrix;
			uboShadow.m_ProjectionMatrix = sunlightProjectionMatrix;
			uboShadow.m_ModelMatrix      = model->GetTransform();
			
			void* data;
			vkMapMemory(logicalDevice, model->GetShadowMemoryBuffer().m_Memory, 0, sizeof(SShadowUniformBuffer), 0, &data);
			memcpy(data, &uboShadow, sizeof(uboShadow));
			vkUnmapMemory(logicalDevice, model->GetShadowMemoryBuffer().m_Memory);
		}

		for (uint32_t i = 0; i < managers->m_Modelmanager->GetNumTerrainModels(); i++)
		{
			CModel* terrainModel = managers->m_Modelmanager->GetTerrainModel(i);

			SShadowUniformBuffer uboShadow{};
			uboShadow.m_ViewMatrix       = sunlightViewMatrix;
			uboShadow.m_ProjectionMatrix = sunlightProjectionMatrix;
			uboShadow.m_ModelMatrix      = terrainModel->GetTransform();
			
			void* data;
			vkMapMemory(logicalDevice, terrainModel->GetShadowMemoryBuffer().m_Memory, 0, sizeof(SShadowUniformBuffer), 0, &data);
			memcpy(data, &uboShadow, sizeof(uboShadow));
			vkUnmapMemory(logicalDevice, terrainModel->GetShadowMemoryBuffer().m_Memory);
		}
	}

	void CShadowNode::Draw(CGraphicsContext* context, SGraphicsManagers* managers, VkCommandBuffer commandBuffer)
	{
		CDebugManager* debugManager = managers->m_DebugManager;

		CResourceManager* resourceManager = managers->m_ResourceManager;
		SRenderResource shadowmapAttachment = resourceManager->TransitionResource(commandBuffer, EResourceIndices::ShadowMap, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

		VkExtent2D prevRenderResolution = context->GetRenderResolution();
		context->SetRenderResolution(VkExtent2D(SHADOWMAP_RESOLUTION, SHADOWMAP_RESOLUTION));

		BeginRendering("Shadow Map", context, commandBuffer, {shadowmapAttachment});

		m_ShadowPipeline->BindPipeline(commandBuffer);

		constexpr float drawNodeMarkerColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
		for (uint32_t i = 0; i < managers->m_Modelmanager->GetNumModels(); i++)
		{
			BeginMarker(context->GetVulkanInstance(), commandBuffer, "Model", drawNodeMarkerColor);
			CModel* model = managers->m_Modelmanager->GetModel(i);

			model->BindVertexAndIndexBuffers(commandBuffer);
			model->BindShadowTable(context, commandBuffer, m_ShadowPipeline->GetPipelineLayout());
			for (uint32_t j = 0; j < model->GetNumMeshes(); j++)
			{
				SMaterialMesh modelMesh = model->GetMesh(j);
				vkCmdDrawIndexed(commandBuffer, modelMesh.m_NumVertices, 1, modelMesh.m_StartIndex, 0, 0);
			}
			EndMarker(context->GetVulkanInstance(), commandBuffer);
		}

		m_TerrainShadowPipeline->BindPipeline(commandBuffer);

		for (uint32_t i = 0; i < managers->m_Modelmanager->GetNumTerrainModels(); i++)
		{
			BeginMarker(context->GetVulkanInstance(), commandBuffer, "Terrain", drawNodeMarkerColor);
			CModel* terrainModel = managers->m_Modelmanager->GetTerrainModel(i);

			terrainModel->BindVertexAndIndexBuffers(commandBuffer);
			terrainModel->BindShadowTable(context, commandBuffer, m_TerrainShadowPipeline->GetPipelineLayout());

			// Each mesh is one triangle strip, see CTerrainNode::Init
			for (uint32_t j = 0; j < terrainModel->GetNumMeshes(); j++)
			{
				SMaterialMesh stripMesh = terrainModel->GetMesh(j);
				vkCmdDrawIndexed(commandBuffer, stripMesh.m_NumVertices, 1, stripMesh.m_StartIndex, 0, 0);
			}
			EndMarker(context->GetVulkanInstance(), commandBuffer);
		}

		EndRendering(context, commandBuffer);

		context->SetRenderResolution(prevRenderResolution);
	}

	void CShadowNode::Cleanup(CGraphicsContext* context)
	{
		vkDestroyBuffer(context->GetLogicalDevice(), m_ShadowBuffer, nullptr);
		vkFreeMemory(context->GetLogicalDevice(), m_ShadowBufferMemory, nullptr);

		m_ShadowPipeline->Cleanup(context);
		m_TerrainShadowPipeline->Cleanup(context);

		delete m_ShadowPipeline;
		delete m_TerrainShadowPipeline;
	}
};