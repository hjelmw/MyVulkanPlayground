#include "TerrainPass.hpp"

namespace NVulkanEngine
{

	struct STerrainVertices
	{

	};

	void CTerrainPass::CreateTerrainVertices()
	{

	}

	void CTerrainPass::InitPass(CGraphicsContext* context, SGraphicsManagers* managers)
	{

	}

	void CTerrainPass::UpdateAtmosphericsConstants(CGraphicsContext* context, SGraphicsManagers* managers)
	{

	}

	void CTerrainPass::DrawPass(CGraphicsContext* context, SGraphicsManagers* managers, VkCommandBuffer commandBuffer)
	{

	}

	void CTerrainPass::CleanupPass(CGraphicsContext* context)
	{
		VkDevice device = context->GetLogicalDevice();

		vkDestroyBuffer(device, m_TerrainUniformBuffer, nullptr);
		vkFreeMemory(device, m_TerrainBufferMemory, nullptr);

		m_TerrainTable->Cleanup(context);
		m_TerrainPipeline->Cleanup(context);

		delete m_TerrainTable;
		delete m_TerrainPipeline;
	}

};