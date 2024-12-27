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

		virtual void InitPass(CGraphicsContext* context, const SGraphicsManagers& managers)  override;
		virtual void Draw(CGraphicsContext* context, const SGraphicsManagers& managers, VkCommandBuffer commandBuffer) override;
		virtual void CleanupPass(CGraphicsContext* context) override;

		static inline SRenderAttachment GetAtmosphericsAttachment() { return s_AtmosphericsAttachment; };
	private:
		void UpdateAtmosphericsBuffer(CGraphicsContext* context, const SGraphicsManagers& managers);

		// Contains the sky box
		static inline SRenderAttachment s_AtmosphericsAttachment;

		std::vector<VkDescriptorSet> m_DescriptorSetsAtmospherics = { };

		// Atmospherics Uniform Buffer
		VkBuffer					m_AtmosphericsBuffer       = VK_NULL_HANDLE;
		VkDeviceMemory				m_AtmosphericsBufferMemory = VK_NULL_HANDLE;

		VkSampler                     m_AtmosphericsSampler      = VK_NULL_HANDLE;


		// Pipeline
		CPipeline* m_AtmosphericsPipeline = nullptr;
	};
}