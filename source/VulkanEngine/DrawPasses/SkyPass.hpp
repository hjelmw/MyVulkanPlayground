#pragma once

#include <DrawPasses/DrawPass.hpp>
#include <DrawPasses/Pipeline.hpp>
#include <DrawPasses/BindingTable.hpp>

namespace NVulkanEngine
{
	class CSkyPass : public CDrawPass
	{
	public:
		CSkyPass() = default;
		~CSkyPass() = default;

		virtual void InitPass(CGraphicsContext* context, SGraphicsManagers* managers)  override;
		virtual void DrawPass(CGraphicsContext* context, SGraphicsManagers* managers, VkCommandBuffer commandBuffer) override;
		virtual void CleanupPass(CGraphicsContext* context) override;

	private:
		void UpdateAtmosphericsConstants(CGraphicsContext* context, SGraphicsManagers* managers);

		VkBuffer       m_AtmosphericsUniformBuffer = VK_NULL_HANDLE;
		VkDeviceMemory m_AtmosphericsBufferMemory  = VK_NULL_HANDLE;

		// Pipeline & shader binding
		CBindingTable* m_AtmosphericsTable         = nullptr;
		CPipeline*     m_AtmosphericsPipeline      = nullptr;
	};
}