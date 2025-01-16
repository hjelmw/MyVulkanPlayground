#pragma once

#include <DrawNodes/DrawNode.hpp>
#include <DrawNodes/Pipeline.hpp>
#include <DrawNodes/BindingTable.hpp>

namespace NVulkanEngine
{
	class CSkyNode : public CDrawNode
	{
	public:
		CSkyNode() = default;
		~CSkyNode() = default;

		virtual void Init(CGraphicsContext* context, SGraphicsManagers* managers)  override;
		virtual void Draw(CGraphicsContext* context, SGraphicsManagers* managers, VkCommandBuffer commandBuffer) override;
		virtual void Cleanup(CGraphicsContext* context) override;

	private:
		void UpdateAtmosphericsConstants(CGraphicsContext* context, SGraphicsManagers* managers);

		VkBuffer       m_AtmosphericsUniformBuffer = VK_NULL_HANDLE;
		VkDeviceMemory m_AtmosphericsBufferMemory  = VK_NULL_HANDLE;

		// Pipeline & shader binding
		CBindingTable* m_AtmosphericsTable         = nullptr;
		CPipeline*     m_AtmosphericsPipeline      = nullptr;
	};
}