#include "GeometryPass.hpp"

#include <glm/glm.hpp>

#ifndef M_PI
#define M_PI 3.1415f
#endif

#include "../InputManager.hpp"
#include "../ModelManager.hpp"

namespace NVulkanEngine
{
	glm::mat4 CGeometryPass::s_SphereMatrix = glm::identity<glm::mat4>();


	// Render targets (outputs)
	enum EGeometryAttachments
	{
		EPositions = 0,
		ENormals = 1,
		EAlbedo = 2,
		EDepth = 3,
		EGeometryAttachmentCount = 4
	};

	struct SGeometryUniformBuffer
	{
		glm::mat4 m_ModelMat;
		glm::mat4 m_ViewMat;
		glm::mat4 m_ProjectionMat;
	};

	void CGeometryPass::InitPass(CGraphicsContext* context)
	{
		s_GeometryAttachments.resize(EGeometryAttachmentCount);

		/* Setup the attachments */
		s_GeometryAttachments[EPositions] = CreateAttachment(
			context,
			VK_FORMAT_R16G16B16A16_SFLOAT,
			VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			context->GetRenderResolution().width,
			context->GetRenderResolution().height);

		s_GeometryAttachments[ENormals] = CreateAttachment(
			context,
			VK_FORMAT_R16G16B16A16_SFLOAT,
			VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			context->GetRenderResolution().width,
			context->GetRenderResolution().height);

		s_GeometryAttachments[EAlbedo] = CreateAttachment(
			context,
			VK_FORMAT_R8G8B8A8_UNORM,
			VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			context->GetRenderResolution().width,
			context->GetRenderResolution().height);

		s_GeometryAttachments[EDepth] = CreateAttachment(
			context,
			FindDepthFormat(context->GetPhysicalDevice()),
			VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
			VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
			context->GetRenderResolution().width,
			context->GetRenderResolution().height);

		/* Setup descriptor bindings, vertex binding and vertex attributes */
		const std::vector<VkDescriptorSetLayoutBinding>      descriptorSetLayoutBindings = CGeometryPass::GetDescriptorSetLayoutBindings();
		const VkVertexInputBindingDescription                vertexBindingDescription    = SVertex::GetVertexBindingDescription();
		const std::vector<VkVertexInputAttributeDescription> vertexAttributeDescriptions = SVertex::GetVertexInputAttributeDescriptions();

		const std::vector<VkFormat> colorAttachmentFormats =
		{
			s_GeometryAttachments[EPositions].m_Format,
			s_GeometryAttachments[ENormals].m_Format,
			s_GeometryAttachments[EAlbedo].m_Format,
		};
		const VkFormat depthFormat = s_GeometryAttachments[EDepth].m_Format;

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

		m_SphereModel = new CModel();
		m_SphereModel->SetModelFilepath("assets/BigSPhere.obj", "./assets");
		m_SphereModel->CreateModelMeshes(context);

		m_ShipModel = new CModel();
		m_ShipModel->SetModelFilepath("assets/NewShip.obj", "./assets");
		m_ShipModel->CreateModelMeshes(context);

		m_FLoorModel = new CModel();
		m_FLoorModel->SetModelFilepath("assets/floor.obj", "./assets");
		m_FLoorModel->CreateModelMeshes(context);

		//m_BoxTexture = new CTexture();
		//m_BoxTexture->SetGenerateMipmaps(false);
		//m_BoxTexture->CreateTexture(context, "assets/box.png", VK_FORMAT_R8G8B8A8_SRGB);

		m_GeometryBufferSphere = CreateBuffer(
			context, 
			m_GeometryBufferMemorySphere,
			sizeof(SGeometryUniformBuffer), 
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, 
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

		m_GeometryBufferFloor = CreateBuffer(
			context,
			m_GeometryBufferMemoryFloor,
			sizeof(SGeometryUniformBuffer),
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

		m_GeometryBufferShip = CreateBuffer(
			context,
			m_GeometryBufferMemoryShip,
			sizeof(SGeometryUniformBuffer),
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
		
		m_GeometrySampler = CreateSampler(
			context, 
			VK_SAMPLER_ADDRESS_MODE_REPEAT, 
			VK_SAMPLER_MIPMAP_MODE_LINEAR, 
			VK_FILTER_LINEAR, 
			VK_FILTER_LINEAR, 
			0.0f, 
			0.0f, 
			1.0f);

		/* Allocate 2 sets per frame in flight consisting of a single uniform buffer and combined image sampler descriptor */
		AllocateDescriptorPool(context, 8, 1, 1);

		CModelManager* modelManager = CModelManager::GetInstance();

		for (uint32_t i = 0; i < modelManager->GetModels().size(); i++)
		{
			CModel* model = modelManager->GetModel(i);

			model->GetDescriptorSets() = AllocateDescriptorSets(context, CDrawPass::m_DescriptorPool, CDrawPass::m_DescriptorSetLayout, g_MaxFramesInFlight);
			
			VkImageView textureImageView = model->GetModelTexture()->GetTextureImageView();
			VkDescriptorImageInfo  descriptorTexture = CreateDescriptorImageInfo(m_GeometrySampler, textureImageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
			VkDescriptorBufferInfo descriptorUniform = CreateDescriptorBufferInfo(m_GeometryBufferShip, sizeof(SGeometryUniformBuffer));

			const std::vector<VkWriteDescriptorSet> writeDescriptorSetShip =
			{
				CreateWriteDescriptorBuffer(context, model->GetDescriptorSets().data(), VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         0, &descriptorUniform),
				CreateWriteDescriptorImage(context,  model->GetDescriptorSets().data(), VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, &descriptorTexture)
			};

			UpdateDescriptorSets(context, model->GetDescriptorSets(), writeDescriptorSetShip);
		}
	}

	void CGeometryPass::UpdateGeometryBuffers(CGraphicsContext* context)
	{
		glm::mat4 sphereMatrix = glm::identity<glm::mat4>();
		glm::mat4 ShipMatrix   = glm::identity<glm::mat4>();
		glm::mat4 floorMatrix  = glm::identity<glm::mat4>();
		
		sphereMatrix = glm::rotate(sphereMatrix, m_RotationDegrees, glm::vec3(0.0f, 1.0f, 0.0f));
		sphereMatrix = glm::translate(sphereMatrix, glm::vec3(0.0f, 20.0f, 25.0f));
		sphereMatrix = glm::scale(sphereMatrix, glm::vec3(0.3f, 0.3f, 0.3f));
		s_SphereMatrix = sphereMatrix;
		m_RotationDegrees = fmod(m_RotationDegrees + 2.0f * context->GetDeltaTime(), 360.0f);

		floorMatrix  = glm::translate(floorMatrix, glm::vec3(0.0f, -5.0f, 0.0f));

		CCamera* camera = CInputManager::GetInstance()->GetCamera();

		{
			SGeometryUniformBuffer uboSphere{};
			uboSphere.m_ModelMat = sphereMatrix;
			uboSphere.m_ViewMat = camera->GetLookAtMatrix();
			uboSphere.m_ProjectionMat = camera->GetProjectionMatrix();

			void* sphereData;
			vkMapMemory(context->GetLogicalDevice(), m_GeometryBufferMemorySphere, 0, sizeof(SGeometryUniformBuffer), 0, &sphereData);
			memcpy(sphereData, &uboSphere, sizeof(uboSphere));
			vkUnmapMemory(context->GetLogicalDevice(), m_GeometryBufferMemorySphere);
		}

		{
			SGeometryUniformBuffer uboFloor{};
			uboFloor.m_ModelMat = floorMatrix;
			uboFloor.m_ViewMat = camera->GetLookAtMatrix();
			uboFloor.m_ProjectionMat = camera->GetProjectionMatrix();

			void* floorData;
			vkMapMemory(context->GetLogicalDevice(), m_GeometryBufferMemoryFloor, 0, sizeof(SGeometryUniformBuffer), 0, &floorData);
			memcpy(floorData, &uboFloor, sizeof(uboFloor));
			vkUnmapMemory(context->GetLogicalDevice(), m_GeometryBufferMemoryFloor);
		}


		{
			SGeometryUniformBuffer uboShip{};
			uboShip.m_ModelMat = ShipMatrix;
			uboShip.m_ViewMat = camera->GetLookAtMatrix();
			uboShip.m_ProjectionMat = camera->GetProjectionMatrix();

			void* ShipData;
			vkMapMemory(context->GetLogicalDevice(), m_GeometryBufferMemoryShip, 0, sizeof(SGeometryUniformBuffer), 0, &ShipData);
			memcpy(ShipData, &uboShip, sizeof(uboShip));
			vkUnmapMemory(context->GetLogicalDevice(), m_GeometryBufferMemoryShip);
		}
	}

	std::vector<VkDescriptorSetLayoutBinding> CGeometryPass::GetDescriptorSetLayoutBindings()
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

	void CGeometryPass::Draw(CGraphicsContext* context, VkCommandBuffer commandBuffer)
	{

		BeginRendering(context, commandBuffer, s_GeometryAttachments);

		m_GeometryPipeline->BindPipeline(commandBuffer);

		UpdateGeometryBuffers(context);

		std::vector<SMesh> sphereMeshes = m_SphereModel->GetMeshes();

		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_PipelineLayout, 0, 1, &m_DescriptorSetsSphere[context->GetFrameIndex()], 0, nullptr);

		for (uint32_t i = 0; i < sphereMeshes.size(); i++)
		{
			SMesh sphereMesh = sphereMeshes[i];
			m_SphereModel->BindMesh(commandBuffer, sphereMesh);

			const SMaterial material = m_SphereModel->GetMaterial(sphereMesh.m_MaterialId);
			vkCmdPushConstants(commandBuffer, CDrawPass::m_PipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, static_cast<uint32_t>(sizeof(SMaterial)), &material);
			vkCmdDrawIndexed(commandBuffer, m_SphereModel->GetNumIndices(), 1, 0, sphereMesh.m_StartIndex, sphereMesh.m_NumVertices);
		}

		std::vector<SMesh> floorMeshes = m_SphereModel->GetMeshes();

		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_PipelineLayout, 0, 1, &m_DescriptorSetFloor[context->GetFrameIndex()], 0, nullptr);

		for (uint32_t i = 0; i < floorMeshes.size(); i++)
		{
			SMesh floorMesh = floorMeshes[i];
			m_FLoorModel->BindMesh(commandBuffer, floorMesh);

			const SMaterial material = m_FLoorModel->GetMaterial(floorMesh.m_MaterialId);
			vkCmdPushConstants(commandBuffer, CDrawPass::m_PipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, static_cast<uint32_t>(sizeof(SMaterial)), &material);
			vkCmdDrawIndexed(commandBuffer, m_FLoorModel->GetNumIndices(), 1, 0, floorMesh.m_StartIndex, floorMesh.m_NumVertices);
		}

		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_PipelineLayout, 0, 1, &m_DescriptorSetsShip[context->GetFrameIndex()], 0, nullptr);

		std::vector<SMesh> shipMeshes = m_ShipModel->GetMeshes();
		m_ShipModel->BindMesh(commandBuffer);

		for (uint32_t i = 0; i < shipMeshes.size(); i++)
		{
			SMesh shipMesh = shipMeshes[i];

			const SMaterial material = m_ShipModel->GetMaterial(shipMesh.m_MaterialId);
			vkCmdPushConstants(commandBuffer, CDrawPass::m_PipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, static_cast<uint32_t>(sizeof(SMaterial)), &material);
			vkCmdDrawIndexed(commandBuffer, shipMesh.m_NumVertices, 1, shipMesh.m_StartIndex, 0, 0);
		}

		EndRendering(commandBuffer);
	}

	void CGeometryPass::CleanupPass(CGraphicsContext* context)
	{
		// Uniform buffer
		vkDestroyBuffer(context->GetLogicalDevice(), m_GeometryBufferSphere, nullptr);
		vkFreeMemory(context->GetLogicalDevice(), m_GeometryBufferMemorySphere, nullptr);

		m_ShipModel->CleanupModel(context);
		m_SphereModel->CleanupModel(context);
		m_FLoorModel->CleanupModel(context);

		// Descriptor pool and layout (don't need to destroy the sets since they are allocated from the pool)
		vkDestroyDescriptorPool(context->GetLogicalDevice(), m_DescriptorPool, nullptr);
		vkDestroyDescriptorSetLayout(context->GetLogicalDevice(), m_DescriptorSetLayout, nullptr);

		// Pipeline and layout
		m_GeometryPipeline->CleanupPipeline(context);
		vkDestroyPipelineLayout(context->GetLogicalDevice(), m_PipelineLayout, nullptr);

		// Attachments
		for (uint32_t i = 0; i < s_GeometryAttachments.size(); i++)
		{
			vkDestroyImageView(context->GetLogicalDevice(), s_GeometryAttachments[i].m_ImageView, nullptr);

			vkDestroyImage(context->GetLogicalDevice(), s_GeometryAttachments[i].m_Image, nullptr);
			vkFreeMemory(context->GetLogicalDevice(), s_GeometryAttachments[i].m_Memory, nullptr);

			s_GeometryAttachments[i].m_Format = VK_FORMAT_UNDEFINED;
			s_GeometryAttachments[i].m_RenderAttachmentInfo = {};
		}
	}
};
