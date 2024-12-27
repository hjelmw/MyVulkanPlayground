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

		virtual void InitPass(CGraphicsContext* context, const SGraphicsManagers& managers)  override;
		virtual void Draw(CGraphicsContext* context, const SGraphicsManagers& managers, VkCommandBuffer commandBuffer) override;
		virtual void CleanupPass(CGraphicsContext* context) override;

		static SRenderAttachment GetSceneColorAttachment() { return s_DeferredAttachments[(uint32_t)ERenderAttachments::SceneColor]; };
		VkDescriptorSet GetImGuiSceneColorDescriptorSet() { return m_ImGuiSceneColorDescriptorSet; };

	private:
		void UpdateLightBuffers(CGraphicsContext* context, const SGraphicsManagers& managers);

		// Deferred lighting pass image attachments
		inline static std::vector<SRenderAttachment> s_DeferredAttachments;

		std::vector<VkDescriptorSet> m_DescriptorSetsLighting = { VK_NULL_HANDLE };

		// Deferred Lighting Uniform Buffer
		VkBuffer					  m_DeferredLightBuffer       = VK_NULL_HANDLE;
		VkDeviceMemory				  m_DeferredLightBufferMemory = VK_NULL_HANDLE;

		VkSampler					  m_DeferredSampler = VK_NULL_HANDLE;
		VkSampler					  m_ClampSampler    = VK_NULL_HANDLE;

		VkDescriptorSet m_ImGuiSceneColorDescriptorSet = VK_NULL_HANDLE;

		// Pipeline
		CPipeline* m_DeferredPipeline = nullptr;
	};
}