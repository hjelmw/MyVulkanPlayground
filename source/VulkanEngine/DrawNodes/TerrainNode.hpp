#pragma once

#include <DrawNodes/DrawNode.hpp>
#include <DrawNodes/Pipeline.hpp>
#include <DrawNodes/BindingTable.hpp>

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
		void UpdateAtmosphericsConstants(CGraphicsContext* context, SGraphicsManagers* managers);
		void CreateTerrainVertices();

		VkBuffer        m_TerrainUniformBuffer = VK_NULL_HANDLE;
		VkDeviceMemory  m_TerrainBufferMemory  = VK_NULL_HANDLE;

		// Terrain vertices
		VkBuffer        m_VertexBuffer         = VK_NULL_HANDLE;
		VkDeviceMemory  m_VertexBufferMemory   = VK_NULL_HANDLE;
		VkBuffer        m_IndexBuffer          = VK_NULL_HANDLE;
		VkDeviceMemory  m_IndexBufferMemory    = VK_NULL_HANDLE;

		// Pipeline & shader binding
		CBindingTable* m_TerrainTable          = nullptr;
		CPipeline*     m_TerrainPipeline       = nullptr;
	};
}