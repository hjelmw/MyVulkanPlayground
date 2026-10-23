#pragma once

#include <Draw/DrawNodes/DrawNode.hpp>

/* 
	Draw stuff on the sky. Currently an atmospheric scattering raymarcher
*/

namespace NVulkanEngine
{
	class CPipeline;
	class CBindingTable;

	class CSkyNode : public CDrawNode
	{
	public:
		CSkyNode() = default;
		~CSkyNode() = default;

		virtual void Init(SRuntimeContext* context, SGraphicsManagers* managers)  override;
		virtual void UpdateBeforeDraw(VkDevice logicalDevice, SGraphicsManagers* managers) override;
		virtual void Draw(SRuntimeContext* context, SGraphicsManagers* managers, VkCommandBuffer commandBuffer) override;
		virtual void Cleanup(SRuntimeContext* context) override;

	private:

		VkBuffer       m_AtmosphericsUniformBuffer = VK_NULL_HANDLE;
		VkDeviceMemory m_AtmosphericsBufferMemory  = VK_NULL_HANDLE;

		// Pipeline & shader binding
		CBindingTable* m_AtmosphericsTable         = nullptr;
		CPipeline*     m_AtmosphericsPipeline      = nullptr;
		CPipeline*     m_VolumetricCloudsPipeline  = nullptr;
	};
}