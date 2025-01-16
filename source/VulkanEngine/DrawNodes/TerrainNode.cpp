#include "TerrainNode.hpp"

namespace NVulkanEngine
{

	struct STerrainVertices
	{

	};

	void CTerrainNode::CreateTerrainVertices()
	{

	}

	void CTerrainNode::Init(CGraphicsContext* context, SGraphicsManagers* managers)
	{

	}

	void CTerrainNode::UpdateAtmosphericsConstants(CGraphicsContext* context, SGraphicsManagers* managers)
	{

	}

	void CTerrainNode::Draw(CGraphicsContext* context, SGraphicsManagers* managers, VkCommandBuffer commandBuffer)
	{

	}

	void CTerrainNode::Cleanup(CGraphicsContext* context)
	{
		//VkDevice device = context->GetLogicalDevice();

		//vkDestroyBuffer(device, m_TerrainUniformBuffer, nullptr);
		//vkFreeMemory(device, m_TerrainBufferMemory, nullptr);

		//m_TerrainTable->Cleanup(context);
		//m_TerrainPipeline->Cleanup(context);

		//delete m_TerrainTable;
		//delete m_TerrainPipeline;
	}

};