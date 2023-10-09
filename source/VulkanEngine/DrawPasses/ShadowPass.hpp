#pragma once

#include <vulkan/vulkan.h>

#include "../DrawPass.hpp"
#include "../Pipeline.hpp"

namespace NVulkanEngine
{
	class CShadowPass : public CDrawPass
	{
	public:
		CShadowPass() = default;
		~CShadowPass() = default;

		virtual void InitPass(CGraphicsContext* context)  override;
		virtual void Draw(CGraphicsContext* context, VkCommandBuffer commandBuffer) override;
		virtual void CleanupPass(CGraphicsContext* context) override;

		// This is the shadow map. Accessed later by deferred lighting pass
		SImageAttachment s_ShadowAttachment;
	private:
		void UpdateShadowBuffers(CGraphicsContext* context);


		std::vector<VkDescriptorSet> m_DescriptorSetsShadow = { VK_NULL_HANDLE };

		// Shadow Uniform Buffer
		VkBuffer					  m_ShadowBuffer       = VK_NULL_HANDLE;
		VkDeviceMemory				  m_ShadowBufferMemory = VK_NULL_HANDLE;

		// Pipeline
		CPipeline* m_ShadowPipeline = nullptr;
	};
}