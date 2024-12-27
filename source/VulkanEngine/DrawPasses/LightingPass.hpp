#pragma once

#include <vulkan/vulkan.h>

#include <DrawPasses/DrawPass.hpp>
#include <DrawPasses/Pipeline.hpp>

namespace NVulkanEngine
{
	class CLightingPass : public CDrawPass
	{
	public:
		CLightingPass() = default;
		~CLightingPass() = default;

		virtual void InitPass(CGraphicsContext* context, SGraphicsManagers* managers)  override;
		virtual void Draw(CGraphicsContext* context, SGraphicsManagers* managers, VkCommandBuffer commandBuffer) override;
		virtual void CleanupPass(CGraphicsContext* context) override;

	private:
		void UpdateLightBuffers(CGraphicsContext* context, SGraphicsManagers* managers);

		std::vector<VkDescriptorSet> m_DescriptorSetsLighting = { VK_NULL_HANDLE };

		// Deferred Lighting Uniform Buffer
		VkBuffer					  m_DeferredLightBuffer       = VK_NULL_HANDLE;
		VkDeviceMemory				  m_DeferredLightBufferMemory = VK_NULL_HANDLE;

		// Pipeline
		CPipeline* m_DeferredPipeline = nullptr;
	};
}