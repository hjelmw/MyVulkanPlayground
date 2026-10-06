#include "TerrainNode.hpp"
#include "Utils/Pipeline.hpp"
#include "Utils/BindingTable.hpp"
#include <VulkanGraphicsEngineUtils.hpp>

namespace NVulkanEngine
{

	struct STerrainVertex
	{
		glm::vec3 m_Position;
		glm::vec3 m_Normal;
	};


	struct STerrainVertexPushConstants
	{
		glm::mat4 m_ViewProjectionMatrix = glm::identity<glm::mat4>();
	};

	struct STerrainFragmentConstants
	{
		float m_TestConst = 0;
	};

	void CTerrainNode::CreateTerrainVertices(CGraphicsContext* context)
	{
		int textureChannels = 0;
		stbi_uc* pixelData = stbi_load("./assets/terrain/iceland_heightmap.png", &m_TerrainTextureWidth, &m_TerrainTextureHeight, &textureChannels, STBI_rgb_alpha);

		if (!pixelData)
		{
			throw std::runtime_error("failed to load terrain texture image!");
		}

		constexpr float terrainHeightScale = 64.0f / 256.0f;
		constexpr float terrainHeightShift = 16.0f;

		auto heightAt = [&](int row, int column)
		{
			row    = glm::clamp(row,    0, m_TerrainTextureHeight - 1);
			column = glm::clamp(column, 0, m_TerrainTextureWidth  - 1);
			return pixelData[(column + m_TerrainTextureWidth * row) * 4] * terrainHeightScale - terrainHeightShift;
		};

		std::vector<STerrainVertex> terrainVertices;
		std::vector<uint32_t> terrainIndices;
		terrainVertices.reserve(m_TerrainTextureHeight * m_TerrainTextureWidth);
		terrainIndices.reserve(m_TerrainTextureHeight  * m_TerrainTextureWidth * 2);

		for (uint32_t i = 0; i < (uint32_t)m_TerrainTextureHeight; i++)
		{
			for (uint32_t j = 0; j < (uint32_t)m_TerrainTextureWidth; j++)
			{
				STerrainVertex terrainVertex{};

				float terrainVertexX = j - m_TerrainTextureWidth / 2.0f;
				float terrainVertexY = heightAt(i, j);
				float terrainVertexZ = i - m_TerrainTextureHeight / 2.0f;
				terrainVertex.m_Position = glm::vec3(terrainVertexX, terrainVertexY, terrainVertexZ);

				float terrainNormalX = heightAt(i, j - 1) - heightAt(i, j + 1);
				float terrainNormalY = 2.0f;
				float terrainNormalZ = heightAt(i - 1, j) - heightAt(i + 1, j);
				terrainVertex.m_Normal = glm::normalize(glm::vec3(terrainNormalX, terrainNormalY, terrainNormalZ));

				terrainVertices.push_back(terrainVertex);
			}
		}

		// One triangle strip per pair of rows
		for (uint32_t i = 0; i < (uint32_t)m_TerrainTextureHeight - 1; i++)
		{
			for (uint32_t j = 0; j < (uint32_t)m_TerrainTextureWidth; j++)
			{
				terrainIndices.push_back(j + m_TerrainTextureWidth * (i + 0));
				terrainIndices.push_back(j + m_TerrainTextureWidth * (i + 1));
			}
		}

		stbi_image_free(pixelData);

		m_NumTerrainVertices = (uint32_t)terrainVertices.size();
		m_NumTerrainIndices  = (uint32_t)terrainIndices.size();

		VkDeviceSize terrainVertexBufferSize = (VkDeviceSize)m_NumTerrainVertices * sizeof(STerrainVertex);
		VkDeviceSize terrainIndexBufferSize  = (VkDeviceSize)m_NumTerrainIndices  * sizeof(uint32_t);

		CreateBufferAndCopyData(
			context, 
			m_TerrainVertexBuffer, 
			m_TerrainVertexBufferMemory, 
			terrainVertices.data(), 
			terrainVertexBufferSize, 
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, 
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
		);

		CreateBufferAndCopyData(
			context, 
			m_TerrainIndexBuffer, 
			m_TerrainIndexBufferMemory, 
			terrainIndices.data(), 
			terrainIndexBufferSize, 
			VK_BUFFER_USAGE_INDEX_BUFFER_BIT, 
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
		);
	}

	void CTerrainNode::Init(CGraphicsContext* context, SGraphicsManagers* managers)
	{
		CreateTerrainVertices(context);

		m_TerrainUniformBuffer = CreateUniformBuffer(context, m_TerrainUniformBufferMemory, sizeof(STerrainFragmentConstants));

		m_TerrainTable = new CBindingTable();
		m_TerrainTable->AddUniformBufferBinding(0, VK_SHADER_STAGE_FRAGMENT_BIT, m_TerrainUniformBuffer, sizeof(STerrainFragmentConstants));
		m_TerrainTable->CreateBindings(context);

		// Same render targets as the geometry node so the deferred lighting pass shades the terrain
		const VkFormat positionsFormat = managers->m_ResourceManager->GetRenderResource(EResourceIndices::Positions).m_Format;
		const VkFormat normalsFormat   = managers->m_ResourceManager->GetRenderResource(EResourceIndices::Normals).m_Format;
		const VkFormat albedoFormat    = managers->m_ResourceManager->GetRenderResource(EResourceIndices::Albedo).m_Format;
		const VkFormat depthFormat     = managers->m_ResourceManager->GetRenderResource(EResourceIndices::Depth).m_Format;

		m_TerrainPipeline = new CPipeline(EPipelineType::GRAPHICS);
		m_TerrainPipeline->SetVertexShader("shaders/terrain.vert.spv");
		m_TerrainPipeline->SetFragmentShader("shaders/terrain.frag.spv");
		m_TerrainPipeline->SetCullingMode(VK_CULL_MODE_BACK_BIT);
		m_TerrainPipeline->SetPrimitiveTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP);
		m_TerrainPipeline->SetVertexInput(sizeof(STerrainVertex), VK_VERTEX_INPUT_RATE_VERTEX);
		m_TerrainPipeline->AddVertexAttribute(0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(STerrainVertex, m_Position));
		m_TerrainPipeline->AddVertexAttribute(1, VK_FORMAT_R32G32B32_SFLOAT, offsetof(STerrainVertex, m_Normal));
		m_TerrainPipeline->AddPushConstantSlot(VK_SHADER_STAGE_VERTEX_BIT, sizeof(STerrainVertexPushConstants), 0);
		m_TerrainPipeline->AddColorAttachment(positionsFormat);
		m_TerrainPipeline->AddColorAttachment(normalsFormat);
		m_TerrainPipeline->AddColorAttachment(albedoFormat);
		m_TerrainPipeline->AddDepthAttachment(depthFormat);
		m_TerrainPipeline->CreatePipeline(context, m_TerrainTable->GetDescriptorSetLayout());
	}

	void CTerrainNode::Draw(CGraphicsContext* context, SGraphicsManagers* managers, VkCommandBuffer commandBuffer)
	{
		CCamera* camera = managers->m_InputManager->GetCamera();
		glm::mat4 cameraViewProjectionMatrix = camera->GetProjectionMatrix() * camera->GetLookAtMatrix();

		STerrainVertexPushConstants terrainPushConstants{};
		terrainPushConstants.m_ViewProjectionMatrix = cameraViewProjectionMatrix;

		// Draw on top of the GBuffer
		CResourceManager* resourceManager = managers->m_ResourceManager;
		SRenderResource positionsAttachment = resourceManager->TransitionResource(commandBuffer, EResourceIndices::Positions, VK_ATTACHMENT_LOAD_OP_LOAD, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
		SRenderResource normalsAttachment   = resourceManager->TransitionResource(commandBuffer, EResourceIndices::Normals,   VK_ATTACHMENT_LOAD_OP_LOAD, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
		SRenderResource albedoAttachment    = resourceManager->TransitionResource(commandBuffer, EResourceIndices::Albedo,    VK_ATTACHMENT_LOAD_OP_LOAD, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
		SRenderResource depthAttachment     = resourceManager->TransitionResource(commandBuffer, EResourceIndices::Depth,     VK_ATTACHMENT_LOAD_OP_LOAD, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);

		BeginRendering("Terrain", context, commandBuffer, { positionsAttachment, normalsAttachment, albedoAttachment, depthAttachment });

		m_TerrainPipeline->BindPipeline(commandBuffer);
		m_TerrainTable->BindTable(context, commandBuffer, m_TerrainPipeline->GetPipelineLayout());
		m_TerrainPipeline->PushConstants(commandBuffer, (void*)&terrainPushConstants);

		VkBuffer vertexBuffer[] = { m_TerrainVertexBuffer };
		VkDeviceSize vertexOffsets[] = { 0 };
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffer, vertexOffsets);
		vkCmdBindIndexBuffer(commandBuffer, m_TerrainIndexBuffer, 0, VK_INDEX_TYPE_UINT32);

		const uint32_t NUM_STRIPS          = m_TerrainTextureHeight - 1;
		const uint32_t NUM_VERTS_PER_STRIP = m_TerrainTextureWidth * 2;

		for (unsigned int strip = 0; strip < NUM_STRIPS; ++strip)
		{
			vkCmdDrawIndexed(commandBuffer, NUM_VERTS_PER_STRIP, 1, (NUM_VERTS_PER_STRIP * strip), 0, 0); // offset to starting index
		}

		EndRendering(context, commandBuffer);
	}

	void CTerrainNode::Cleanup(CGraphicsContext* context)
	{
		vkDestroyBuffer(context->GetLogicalDevice(), m_TerrainVertexBuffer, nullptr);
		vkFreeMemory(context->GetLogicalDevice(), m_TerrainVertexBufferMemory, nullptr);

		vkDestroyBuffer(context->GetLogicalDevice(), m_TerrainIndexBuffer, nullptr);
		vkFreeMemory(context->GetLogicalDevice(), m_TerrainIndexBufferMemory, nullptr);

		vkDestroyBuffer(context->GetLogicalDevice(), m_TerrainUniformBuffer, nullptr);
		vkFreeMemory(context->GetLogicalDevice(), m_TerrainUniformBufferMemory, nullptr);

		m_TerrainTable->Cleanup(context);
		m_TerrainPipeline->Cleanup(context);

		delete m_TerrainTable;
	}

};