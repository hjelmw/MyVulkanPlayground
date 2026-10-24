#include "TerrainNode.hpp"
#include "Utils/Pipeline.hpp"
#include "ShaderConstants.hpp"



#include <imgui.h>

static float g_TerrainScale = 64.0f;

namespace NVulkanEngine
{

	struct STerrainVertex
	{
		glm::vec3 m_Position;
		glm::vec3 m_Normal;
	};

	size_t CTerrainNode::GetVertexStride()
	{
		return sizeof(STerrainVertex);
	}

	void CTerrainNode::CreateTerrainVertices(SRuntimeContext* context)
	{
		int textureChannels = 0;
		stbi_uc* pixelData = stbi_load("./assets/terrain/iceland_heightmap.png", &m_TerrainTextureWidth, &m_TerrainTextureHeight, &textureChannels, STBI_rgb_alpha);

		if (!pixelData)
		{
			throw std::runtime_error("failed to load terrain texture image!");
		}

		constexpr float terrainHeightScale = 0.25f;
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
				m_TerrainBounds.extend(terrainVertex.m_Position);

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

	void CTerrainNode::Init(SRuntimeContext* context, SGraphicsManagers* managers)
	{
		CreateTerrainVertices(context);

		m_TerrainTransform = glm::scale(glm::identity<glm::mat4>(), glm::vec3(g_TerrainScale, g_TerrainScale, g_TerrainScale));

		// Register the terrain as a model so the shadow pass can draw it as well. One mesh per triangle strip
		m_ModelManager = managers->m_Modelmanager;
		m_TerrainModel = m_ModelManager->RegisterTerrainModel(m_TerrainVertexBuffer, m_TerrainIndexBuffer, m_NumTerrainVertices, m_NumTerrainIndices);
		if (m_TerrainModel)
		{
			const uint32_t numStrips          = m_TerrainTextureHeight - 1;
			const uint32_t numIndicesPerStrip = m_TerrainTextureWidth * 2;
			for (uint32_t strip = 0; strip < numStrips; strip++)
			{
				SMaterialMesh stripMesh{};
				stripMesh.m_StartIndex  = numIndicesPerStrip * strip;
				stripMesh.m_NumVertices = numIndicesPerStrip;
				m_TerrainModel->AddMesh(stripMesh);
			}

			glm::AABB terrainWorldBounds = m_TerrainBounds;
			terrainWorldBounds.transformCorners(m_TerrainTransform);

			m_TerrainModel->SetTransform(m_TerrainTransform);
			m_TerrainModel->SetAABB(terrainWorldBounds);

			// The shadow map frustum is fitted to the scene bounds so they need to include the terrain
			glm::AABB sceneBounds = m_ModelManager->GetSceneBounds();
			sceneBounds.extend(terrainWorldBounds);
			m_ModelManager->SetSceneBounds(sceneBounds);
		}


		// Same render targets as the geometry node so the deferred lighting pass shades the terrain
		const VkFormat positionsFormat = managers->m_ResourceManager->Get<NRenderResources::SGBufferPositions>().m_Format;
		const VkFormat normalsFormat   = managers->m_ResourceManager->Get<NRenderResources::SGBufferNormals>().m_Format;
		const VkFormat albedoFormat    = managers->m_ResourceManager->Get<NRenderResources::SGBufferAlbedo>().m_Format;
		const VkFormat depthFormat     = managers->m_ResourceManager->Get<NRenderResources::SGBufferDepth>().m_Format;

		m_TerrainPipeline = new CPipeline(EPipelineType::GRAPHICS);
		m_TerrainPipeline->SetDebugName("Terrain");
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
		m_TerrainPipeline->CreatePipeline(context, VK_NULL_HANDLE); // No descriptor sets
	}

	void CTerrainNode::Draw(SRuntimeContext* context, SGraphicsManagers* managers, VkCommandBuffer commandBuffer)
	{
		ImGui::Begin("Terrain Node");
		ImGui::SliderFloat("Terrain Scale", &g_TerrainScale, 1.0f, 128.0f);
		ImGui::End();

		CCamera* camera = managers->m_InputManager->GetCamera();
		glm::mat4 cameraViewProjectionMatrix = camera->GetProjectionMatrix() * camera->GetLookAtMatrix();

		// Model matrix is separate so the GBuffer gets world space positions for lighting and shadows
		STerrainVertexPushConstants terrainPushConstants{};
		terrainPushConstants.m_ViewProjectionMatrix = cameraViewProjectionMatrix;
		terrainPushConstants.m_ModelMatrix          = m_TerrainTransform;

		// Draw on top of the GBuffer
		CResourceManager* resourceManager = managers->m_ResourceManager;
		SRenderResource positionsAttachment = resourceManager->TransitionResource<NRenderResources::SGBufferPositions>(commandBuffer, VK_ATTACHMENT_LOAD_OP_LOAD, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
		SRenderResource normalsAttachment   = resourceManager->TransitionResource<NRenderResources::SGBufferNormals>(commandBuffer, VK_ATTACHMENT_LOAD_OP_LOAD, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
		SRenderResource albedoAttachment    = resourceManager->TransitionResource<NRenderResources::SGBufferAlbedo>(commandBuffer, VK_ATTACHMENT_LOAD_OP_LOAD, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
		SRenderResource depthAttachment     = resourceManager->TransitionResource<NRenderResources::SGBufferDepth>(commandBuffer, VK_ATTACHMENT_LOAD_OP_LOAD, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);

		BeginRendering("Terrain", context, commandBuffer, { positionsAttachment, normalsAttachment, albedoAttachment, depthAttachment });

		m_TerrainPipeline->BindPipeline(commandBuffer);
		m_TerrainPipeline->PushConstants(commandBuffer, (void*)&terrainPushConstants);

		VkBuffer vertexBuffer[] = { m_TerrainVertexBuffer };
		VkDeviceSize vertexOffsets[] = { 0 };
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffer, vertexOffsets);
		vkCmdBindIndexBuffer(commandBuffer, m_TerrainIndexBuffer, 0, VK_INDEX_TYPE_UINT32);

		const uint32_t NUM_STRIPS          = m_TerrainTextureHeight - 1;
		const uint32_t NUM_VERTS_PER_STRIP = m_TerrainTextureWidth * 2;

		for (uint32_t strip = 0; strip < NUM_STRIPS; ++strip)
		{
			vkCmdDrawIndexed(commandBuffer, NUM_VERTS_PER_STRIP, 1, (NUM_VERTS_PER_STRIP * strip), 0, 0); // offset to starting index
		}

		EndRendering(context, commandBuffer);
	}

	void CTerrainNode::Cleanup(SRuntimeContext* context)
	{
		// Draw nodes are recreated on resize, so don't leave a terrain model with destroyed buffers behind
		if (m_TerrainModel)
		{
			m_ModelManager->UnregisterTerrainModel(context, m_TerrainModel);
			m_TerrainModel = nullptr;
		}

		vkDestroyBuffer(context->m_VulkanDevice, m_TerrainVertexBuffer, nullptr);
		vkFreeMemory(context->m_VulkanDevice, m_TerrainVertexBufferMemory, nullptr);

		vkDestroyBuffer(context->m_VulkanDevice, m_TerrainIndexBuffer, nullptr);
		vkFreeMemory(context->m_VulkanDevice, m_TerrainIndexBufferMemory, nullptr);

		m_TerrainPipeline->Cleanup(context);
	}

};