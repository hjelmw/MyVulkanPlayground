#pragma once

#include <DrawPasses/DrawPass.hpp>
#include <DrawPasses/Pipeline.hpp>
#include <DrawPasses/BindingTable.hpp>

namespace NVulkanEngine
{
	class CLightingPass : public CDrawPass
	{
	public:
		CLightingPass() = default;
		~CLightingPass() = default;

		virtual void InitPass(CGraphicsContext* context, SGraphicsManagers* managers)  override;
		virtual void DrawPass(CGraphicsContext* context, SGraphicsManagers* managers, VkCommandBuffer commandBuffer) override;
		virtual void CleanupPass(CGraphicsContext* context) override;

	private:
		void UpdateLightBuffers(CGraphicsContext* context, SGraphicsManagers* managers);

		VkBuffer       m_DeferredUniformBuffer       = VK_NULL_HANDLE;
		VkDeviceMemory m_DeferredLightBufferMemory   = VK_NULL_HANDLE;

		// Pipeline & shader binding
		CBindingTable* m_DeferredTable               = nullptr;
		CPipeline*     m_DeferredPipeline            = nullptr;
	};
}