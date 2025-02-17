#pragma once

#include <DrawNodes/DrawNode.hpp>
#include <DrawNodes/Pipeline.hpp>
#include <DrawNodes/BindingTable.hpp>

/*
	Does deferred lighting
*/

namespace NVulkanEngine
{
	class CLightingNode : public CDrawNode
	{
	public:
		CLightingNode() = default;
		~CLightingNode() = default;

		virtual void Init(CGraphicsContext * context, SGraphicsManagers * managers)  override;
		virtual void Draw(CGraphicsContext* context, SGraphicsManagers* managers, VkCommandBuffer commandBuffer) override;
		virtual void Cleanup(CGraphicsContext* context) override;

	private:
		void UpdateLightBuffers(CGraphicsContext* context, SGraphicsManagers* managers);

		VkBuffer       m_DeferredUniformBuffer       = VK_NULL_HANDLE;
		VkDeviceMemory m_DeferredLightBufferMemory   = VK_NULL_HANDLE;

		// Pipeline & shader binding
		CBindingTable* m_DeferredTable               = nullptr;
		CPipeline*     m_DeferredPipeline            = nullptr;
	};
}