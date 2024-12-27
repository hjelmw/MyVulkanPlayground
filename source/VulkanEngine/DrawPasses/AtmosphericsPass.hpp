#pragma once

#include <vulkan/vulkan.h>

#include <DrawPasses/DrawPass.hpp>
#include <DrawPasses/Pipeline.hpp>
#include <Managers/Model.hpp>

namespace NVulkanEngine
{
	class CAtmosphericsPass : public CDrawPass
	{
	public:
		CAtmosphericsPass() = default;
		~CAtmosphericsPass() = default;

		virtual void InitPass(CGraphicsContext* context, SGraphicsManagers* managers)  override;
		virtual void Draw(CGraphicsContext* context, SGraphicsManagers* managers, VkCommandBuffer commandBuffer) override;
		virtual void CleanupPass(CGraphicsContext* context) override;

	private:
		void UpdateAtmosphericsBuffer(CGraphicsContext* context, SGraphicsManagers* managers);

		std::vector<VkDescriptorSet> m_DescriptorSetsAtmospherics = { };

		// Atmospherics Uniform Buffer
		VkBuffer					m_AtmosphericsBuffer       = VK_NULL_HANDLE;
		VkDeviceMemory				m_AtmosphericsBufferMemory = VK_NULL_HANDLE;

		// Pipeline
		CPipeline* m_AtmosphericsPipeline = nullptr;
	};
}