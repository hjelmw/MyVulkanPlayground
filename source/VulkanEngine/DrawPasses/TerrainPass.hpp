#pragma once

#include <DrawPasses/DrawPass.hpp>
#include <DrawPasses/Pipeline.hpp>
#include <DrawPasses/BindingTable.hpp>

namespace NVulkanEngine
{
	class CTerrainPass : public CDrawPass
	{
	public:
		CTerrainPass() = default;
		~CTerrainPass() = default;

		virtual void InitPass(CGraphicsContext* context, SGraphicsManagers* managers)  override;
		virtual void DrawPass(CGraphicsContext* context, SGraphicsManagers* managers, VkCommandBuffer commandBuffer) override;
		virtual void CleanupPass(CGraphicsContext* context) override;

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