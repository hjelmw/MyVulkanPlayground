#pragma once

#include <vulkan/vulkan.h>

#include "../DrawPass.hpp"
#include "../Pipeline.hpp"

namespace NVulkanEngine
{
	class CLightingPass : public CDrawPass
	{
	public:
		CLightingPass() = default;
		~CLightingPass() = default;

		virtual void InitPass(CGraphicsContext* context)  override;
		virtual void Draw(CGraphicsContext* context, VkCommandBuffer commandBuffer) override;
		virtual void CleanupPass(CGraphicsContext* context) override;

	private:
		void UpdateLightBuffers(CGraphicsContext* context);

		const std::vector<VkDescriptorSetLayoutBinding> GetDescriptorSetLayoutBindings();

		// Deferred lighting pass image attachments
		std::vector<SImageAttachment> m_DeferredAttachments;

		std::vector<VkDescriptorSet> m_DescriptorSetsLighting = { VK_NULL_HANDLE };

		// Deferred Lighting Uniform Buffer
		VkBuffer					  m_DeferredLightBuffer       = VK_NULL_HANDLE;
		VkDeviceMemory				  m_DeferredLightBufferMemory = VK_NULL_HANDLE;

		VkSampler					  m_DeferredSampler = VK_NULL_HANDLE;

		// Pipeline
		CPipeline* m_DeferredPipeline = nullptr;
	};
}