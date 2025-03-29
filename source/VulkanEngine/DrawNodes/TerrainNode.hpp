#pragma once

#include <DrawNodes/DrawNode.hpp>
#include <DrawNodes/Utils/Pipeline.hpp>
#include <DrawNodes/Utils/BindingTable.hpp>

/*
	Draw heightmap terrain. Implementation TBD so currently does nothing hihi
*/

namespace NVulkanEngine
{
	class CTerrainNode : public CDrawNode
	{
	public:
		CTerrainNode() = default;
		~CTerrainNode() = default;

		virtual void Init(CGraphicsContext* context, SGraphicsManagers* managers)  override;
		virtual void Draw(CGraphicsContext* context, SGraphicsManagers* managers, VkCommandBuffer commandBuffer) override;
		virtual void Cleanup(CGraphicsContext* context) override;

	private:
		void UpdateTerrainConstants(CGraphicsContext* context, SGraphicsManagers* managers);
		void CreateTerrainVertices(CGraphicsContext* context);

		VkBuffer        m_TerrainUniformBuffer = VK_NULL_HANDLE;
		VkDeviceMemory  m_TerrainUniformBufferMemory  = VK_NULL_HANDLE;

		VkBuffer        m_TerrainVertexBuffer       = VK_NULL_HANDLE;
		VkDeviceMemory  m_TerrainVertexBufferMemory = VK_NULL_HANDLE;

		VkBuffer        m_TerrainIndexBuffer        = VK_NULL_HANDLE;
		VkDeviceMemory  m_TerrainIndexBufferMemory  = VK_NULL_HANDLE;

		// Pipeline & shader binding
		CPipeline*     m_TerrainPipeline       = nullptr;

		int m_TerrainTextureWidth       = -1;
		int m_TerrainTextureHeight      = -1;
		uint32_t m_NumTerrainVertices   = 0;
		uint32_t m_NumTerrainIndices    = 0;
	};
}