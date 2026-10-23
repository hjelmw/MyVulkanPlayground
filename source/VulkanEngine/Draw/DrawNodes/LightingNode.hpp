#pragma once

#include <Draw/DrawNodes/DrawNode.hpp>

/*
	Reads G-buffers and does deferred lighting
*/

namespace NVulkanEngine
{
	class CPipeline;
	class CBindingTable;

	class CLightingNode : public CDrawNode
	{
	public:
		CLightingNode() = default;
		~CLightingNode() = default;

		virtual void Init(SRuntimeContext * context, SGraphicsManagers * managers)  override;
		virtual void UpdateBeforeDraw(VkDevice logicalDevice, SGraphicsManagers* managers) override;
		virtual void Draw(SRuntimeContext* context, SGraphicsManagers* managers, VkCommandBuffer commandBuffer) override;
		virtual void Cleanup(SRuntimeContext* context) override;

	private:

		VkBuffer       m_DeferredUniformBuffer       = VK_NULL_HANDLE;
		VkDeviceMemory m_DeferredLightBufferMemory   = VK_NULL_HANDLE;

		// Pipeline & shader binding
		CBindingTable* m_DeferredTable               = nullptr;
		CPipeline*     m_DeferredPipeline            = nullptr;
	};
}