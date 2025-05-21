#include "TerrainNode.hpp"
#include <VulkanGraphicsEngineUtils.hpp>

namespace NVulkanEngine
{

	struct STerrainVertices
	{
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

		constexpr float terrainHeightScale = 64.0f;
		constexpr float terrainHeightShift = 16.0f;

		std::vector<glm::vec3> terrainVertices;	
		std::vector<uint32_t> terrainIndices;
		terrainVertices.reserve(m_TerrainTextureHeight * m_TerrainTextureWidth);
		terrainIndices.reserve(m_TerrainTextureHeight * m_TerrainTextureWidth * 2);

		for (uint32_t i = 0; i < (uint32_t)m_TerrainTextureHeight; i++)
		{
			for (uint32_t j = 0; j < (uint32_t)m_TerrainTextureWidth; j++)
			{
				unsigned char* terrainHeightMapValue = pixelData + (j + m_TerrainTextureWidth * i) * textureChannels;

				float terrainVertexX = -m_TerrainTextureHeight / 2.0f + m_TerrainTextureHeight * i / (float) m_TerrainTextureHeight;
				float terrainVertexY = (int) terrainHeightMapValue[0] * terrainHeightScale - terrainHeightShift;
				float terrainVertexZ = -m_TerrainTextureWidth / 2.0f + m_TerrainTextureWidth * j / (float) m_TerrainTextureWidth;
				glm::vec3 terrainVertex = glm::vec3(terrainVertexX, terrainVertexY, terrainVertexZ);

				terrainVertices.push_back(terrainVertex);
				terrainIndices.push_back(j + m_TerrainTextureWidth * (i + 0));
				terrainIndices.push_back(j + m_TerrainTextureWidth * (i + 1));
			}
		}

		stbi_image_free(pixelData);

		m_NumTerrainVertices = (uint32_t)terrainVertices.size();
		m_NumTerrainIndices  = (uint32_t)terrainIndices.size();


		VkDeviceSize terrainVertexBufferSize = (VkDeviceSize)m_NumTerrainVertices * sizeof(glm::vec3);
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

		const SRenderResource sceneColorAttachment = managers->m_ResourceManager->GetRenderResource(EResourceIndices::SceneColor);
		const SRenderResource depthAttachment      = managers->m_ResourceManager->GetRenderResource(EResourceIndices::Depth);


		m_TerrainPipeline = new CPipeline(EPipelineType::GRAPHICS);
		m_TerrainPipeline->SetVertexShader("shaders/terrain.vert.spv");
		m_TerrainPipeline->SetFragmentShader("shaders/terrain.frag.spv");
		m_TerrainPipeline->SetCullingMode(VK_CULL_MODE_BACK_BIT);
		m_TerrainPipeline->SetPrimitiveTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP);
		m_TerrainPipeline->SetVertexInput(sizeof(glm::vec3), VK_VERTEX_INPUT_RATE_VERTEX);
		m_TerrainPipeline->AddVertexAttribute(0, VK_FORMAT_R32G32B32_SFLOAT, 0);
		m_TerrainPipeline->AddSampledBufferBinding(0, VK_SHADER_STAGE_FRAGMENT_BIT, m_TerrainUniformBuffer, sizeof(STerrainFragmentConstants));
		m_TerrainPipeline->AddPushConstantSlot(VK_SHADER_STAGE_VERTEX_BIT, sizeof(STerrainVertexPushConstants), 0);
		m_TerrainPipeline->AddColorAttachment(sceneColorAttachment.m_Format);
		m_TerrainPipeline->AddDepthAttachment(depthAttachment.m_Format);
		m_TerrainPipeline->CreatePipeline(context);
	}

	void CTerrainNode::UpdateTerrainConstants(CGraphicsContext* context, SGraphicsManagers* managers)
	{

	}

	void CTerrainNode::Draw(CGraphicsContext* context, SGraphicsManagers* managers, VkCommandBuffer commandBuffer)
	{
		UpdateTerrainConstants(context, managers);

		//glm::mat4 cameraLookAt = managers->m_InputManager->GetCamera()->GetLookAtMatrix();
		//glm::mat4 cameraProjection = managers->m_InputManager->GetCamera()->GetProjectionMatrix();
		glm::mat4 cameraLookAt = glm::lookAt(glm::vec3(67.0f, 627.5f, 170.0f), glm::vec3(67.0f, 627.5f, 170.0f) + glm::vec3(-0.45f, -0.67f, -0.58f), glm::vec3(-0.41f, 0.73f, -0.53f));
		glm::mat4 cameraProjection = glm::perspective(glm::radians(45.0f), (float)g_DisplayWidth / (float)g_DisplayHeight, 0.1f, 100000.0f);

		glm::mat4 cameraViewProjectionMatrix = cameraProjection * cameraLookAt;
		STerrainVertexPushConstants terrainPushConstants{};
		terrainPushConstants.m_ViewProjectionMatrix = cameraViewProjectionMatrix;

		CResourceManager* resourceManager = managers->m_ResourceManager;
		resourceManager->TransitionResource(commandBuffer, EResourceIndices::Depth, VK_ATTACHMENT_LOAD_OP_LOAD, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);

		SRenderResource depthAttachment = resourceManager->TransitionResource(commandBuffer, EResourceIndices::Depth, VK_ATTACHMENT_LOAD_OP_LOAD, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);
		SRenderResource sceneColorAttachment = resourceManager->TransitionResource(commandBuffer, EResourceIndices::SceneColor, VK_ATTACHMENT_LOAD_OP_LOAD, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

		BeginRendering("Terrain", context, commandBuffer, { sceneColorAttachment, depthAttachment });

		m_TerrainPipeline->BindPipeline(context, commandBuffer);
		m_TerrainPipeline->PushConstants(commandBuffer, (void*)&terrainPushConstants);

		VkBuffer vertexBuffer[] = { m_TerrainVertexBuffer };
		VkDeviceSize vertexOffsets[] = { 0 };
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffer, vertexOffsets);
		vkCmdBindIndexBuffer(commandBuffer, m_TerrainIndexBuffer, 0, VK_INDEX_TYPE_UINT32);

		const uint32_t NUM_STRIPS          = m_TerrainTextureHeight - 1;
		const uint32_t NUM_VERTS_PER_STRIP = m_TerrainTextureWidth * 2;

		for (unsigned int strip = 0; strip < NUM_STRIPS; ++strip)
		{
			//vkCmdDrawIndexed(commandBuffer, NUM_VERTS_PER_STRIP, 1, (NUM_VERTS_PER_STRIP * strip), 0, 0); // offset to starting index
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

		m_TerrainPipeline->Cleanup(context);
	}

};