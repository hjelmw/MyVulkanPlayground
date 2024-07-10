#pragma once

#include <vulkan/vulkan.h>

#include "../DrawPass.hpp"
#include "../Pipeline.hpp"
#include "../Model.hpp"

namespace NVulkanEngine
{
	class CAtmosphericsPass : public CDrawPass
	{
	public:
		CAtmosphericsPass() = default;
		~CAtmosphericsPass() = default;

		virtual void InitPass(CGraphicsContext* context)  override;
		virtual void Draw(CGraphicsContext* context, VkCommandBuffer commandBuffer) override;
		virtual void CleanupPass(CGraphicsContext* context) override;

		inline static SImageAttachment GetAtmosphericInscatteringAttachment() { return s_InscatteringAttachment; }

	private:
		void UpdateAtmosphericsBuffer(CGraphicsContext* context);

		std::vector<SDescriptorSets> m_DescriptorSetsAtmospherics = { };

		// Atmospherics Uniform Buffer
		VkBuffer					  m_AtmosphericsBuffer       = VK_NULL_HANDLE;
		VkDeviceMemory				  m_AtmosphericsBufferMemory = VK_NULL_HANDLE;

		inline static SImageAttachment s_InscatteringAttachment;

		// Pipeline
		CPipeline* m_AtmosphericsPipeline = nullptr;
	};
}