#include "GeometryPass.hpp"

#include <imgui.h>

#ifndef M_PI
#define M_PI 3.1415f
#endif

#include "../InputManager.hpp"
#include "../ModelManager.hpp"

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

	void CGeometryPass::InitPass(CGraphicsContext* context)
	{
		s_GeometryAttachments.resize(4);

		/* Setup the attachments */
		s_GeometryAttachments[(uint32_t)ERenderAttachments::Positions] = CreateRenderAttachment(
			context,
			VK_FORMAT_R16G16B16A16_SFLOAT,
			VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			context->GetRenderResolution().width,
			context->GetRenderResolution().height);

		s_GeometryAttachments[(uint32_t)ERenderAttachments::Normals] = CreateRenderAttachment(
			context,
			VK_FORMAT_R16G16B16A16_SFLOAT,
			VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			context->GetRenderResolution().width,
			context->GetRenderResolution().height);

		s_GeometryAttachments[(uint32_t)ERenderAttachments::Albedo] = CreateRenderAttachment(
			context,
			VK_FORMAT_R8G8B8A8_UNORM,
			VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			context->GetRenderResolution().width,
			context->GetRenderResolution().height);

		s_GeometryAttachments[(uint32_t)ERenderAttachments::Depth] = CreateRenderAttachment(
			context,
			FindDepthFormat(context->GetPhysicalDevice()),
			VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
			VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
			context->GetRenderResolution().width,
			context->GetRenderResolution().height);

		/* Setup descriptor bindings, vertex binding and vertex attributes */
		const std::vector<VkDescriptorSetLayoutBinding>      descriptorSetLayoutBindings = GetDescriptorSetLayoutBindings();
		const VkVertexInputBindingDescription                vertexBindingDescription    = SVertex::GetVertexBindingDescription();
		const std::vector<VkVertexInputAttributeDescription> vertexAttributeDescriptions = SVertex::GetModelVertexInputAttributes();

		const std::vector<VkFormat> colorAttachmentFormats =
		{
			s_GeometryAttachments[(uint32_t)ERenderAttachments::Positions].m_Format,
			s_GeometryAttachments[(uint32_t)ERenderAttachments::Normals].m_Format,
			s_GeometryAttachments[(uint32_t)ERenderAttachments::Albedo].m_Format,
		};
		const VkFormat depthFormat = s_GeometryAttachments[(uint32_t)ERenderAttachments::Depth].m_Format;

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
		
		m_GeometrySampler = CreateSampler(
			context,
			VK_SAMPLER_ADDRESS_MODE_REPEAT,
			VK_SAMPLER_ADDRESS_MODE_REPEAT,
			VK_SAMPLER_ADDRESS_MODE_REPEAT,
			VK_SAMPLER_MIPMAP_MODE_LINEAR, 
			VK_FILTER_LINEAR,
			VK_FILTER_LINEAR,
			0.0f,
			0.0f,
			1.0f);

		CModelManager* modelManager = CModelManager::GetInstance();

		/* Allocate 2 sets per frame in flight consisting of a single uniform buffer and combined image sampler descriptor */
		AllocateDescriptorPool(context, modelManager->GetNumModels() * 2, modelManager->GetNumModels() * 2, modelManager->GetNumModels() * 2);

		for (uint32_t i = 0; i < modelManager->GetNumModels(); i++)
		{
			CModel* model = modelManager->GetModel(i);

			SDescriptorSets& modelDescriptorRef = model->GetDescriptorSetsRef();

			modelDescriptorRef.m_DescriptorSets = AllocateDescriptorSets(context, CDrawPass::m_DescriptorPool, CDrawPass::m_DescriptorSetLayout, g_MaxFramesInFlight);
			model->CreateGeometryMemoryBuffer(context, (VkDeviceSize)sizeof(SGeometryUniformBuffer));

			VkDescriptorBufferInfo descriptorUniform = CreateDescriptorBufferInfo(model->GetGeometryMemoryBuffer().m_Buffer, sizeof(SGeometryUniformBuffer));
			VkDescriptorImageInfo descriptorTexture = CreateDescriptorImageInfo(m_GeometrySampler, model->UsesModelTexture() ? model->GetModelTexture()->GetTextureImageView() : VK_NULL_HANDLE, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

			modelDescriptorRef.m_WriteDescriptors =
			{
				CreateWriteDescriptorBuffer(context, modelDescriptorRef.m_DescriptorSets.data(), VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, &descriptorUniform),
				CreateWriteDescriptorImage(context,  modelDescriptorRef.m_DescriptorSets.data(), VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, &descriptorTexture),
			};

			UpdateDescriptorSets(context, modelDescriptorRef.m_DescriptorSets, modelDescriptorRef.m_WriteDescriptors);
		}
	}

	void CGeometryPass::UpdateGeometryBuffers(CGraphicsContext* context)
	{
		//m_RotationDegrees = fmod(m_RotationDegrees + 2.0f * context->GetDeltaTime(), 360.0f);

		glm::mat4 sphereMatrix = glm::identity<glm::mat4>();
		sphereMatrix = glm::translate(sphereMatrix, glm::vec3(g_LightPosition[0], g_LightPosition[1], g_LightPosition[2]));
		s_SphereMatrix = sphereMatrix;

		CCamera* camera = CInputManager::GetInstance()->GetCamera();

		CModelManager* modelManager = CModelManager::GetInstance();

		for (uint32_t i = 0; i < modelManager->GetNumModels(); i++)
		{
			CModel* model = modelManager->GetModel(i);

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

	void CGeometryPass::Draw(CGraphicsContext* context, VkCommandBuffer commandBuffer)
	{
		ImGui::Begin("Geometry Pass");
		ImGui::SliderFloat3("Light Position", g_LightPosition, -500.0f, 500.0f);
		ImGui::End();

		BeginRendering(context, commandBuffer, s_GeometryAttachments);
		UpdateGeometryBuffers(context);

		m_GeometryPipeline->Bind(commandBuffer);


		CModelManager* modelManager = CModelManager::GetInstance();
		for (uint32_t i = 0; i < modelManager->GetNumModels(); i++)
		{
			CModel* model = modelManager->GetModel(i);

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

		// Uniform buffer
		vkDestroyBuffer(device, m_GeometryBufferSphere, nullptr);
		vkFreeMemory(device, m_GeometryBufferMemorySphere, nullptr);

		// Descriptor pool and layout (don't need to destroy the sets since they are allocated from the pool)
		vkDestroyDescriptorPool(device, m_DescriptorPool, nullptr);
		vkDestroyDescriptorSetLayout(device, m_DescriptorSetLayout, nullptr);

		// Pipeline and layout
		m_GeometryPipeline->Cleanup(context);
		vkDestroyPipelineLayout(device, m_PipelineLayout, nullptr);

		// Attachments
		for (uint32_t i = 0; i < s_GeometryAttachments.size(); i++)
		{
			vkDestroyImageView(device, s_GeometryAttachments[i].m_ImageView, nullptr);

			vkDestroyImage(device, s_GeometryAttachments[i].m_Image, nullptr);
			vkFreeMemory(device, s_GeometryAttachments[i].m_Memory, nullptr);

			s_GeometryAttachments[i].m_Format = VK_FORMAT_UNDEFINED;
			s_GeometryAttachments[i].m_RenderAttachmentInfo = {};
		}
	}
};
