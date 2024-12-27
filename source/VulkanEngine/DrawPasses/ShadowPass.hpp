#pragma once

#include <vulkan/vulkan.h>

#include <DrawPasses/DrawPass.hpp>
#include <DrawPasses/Pipeline.hpp>
#include <Managers/Model.hpp>

namespace NVulkanEngine
{
	class CShadowPass : public CDrawPass
	{
	public:
		CShadowPass() = default;
		~CShadowPass() = default;

		virtual void InitPass(CGraphicsContext* context, const SGraphicsManagers& managers)  override;
		virtual void Draw(CGraphicsContext* context, const SGraphicsManagers& managers, VkCommandBuffer commandBuffer) override;
		virtual void CleanupPass(CGraphicsContext* context) override;

		inline static SRenderAttachment GetShadowMapAttachment() { return s_ShadowAttachment; }
		static glm::mat4 GetLightMatrix() { return s_LightMatrix; };


	private:
		void UpdateShadowBuffers(CGraphicsContext* context, const SGraphicsManagers& managers);

		std::vector<SDescriptorSets> m_DescriptorSetsShadow = { };

		// Shadow Uniform Buffer
		VkBuffer					  m_ShadowBuffer       = VK_NULL_HANDLE;
		VkDeviceMemory				  m_ShadowBufferMemory = VK_NULL_HANDLE;

		// This is the shadow map. Used by deferred lighting pass
		inline static SRenderAttachment s_ShadowAttachment;

		static glm::mat4              s_LightMatrix;

		// Pipeline
		CPipeline* m_ShadowPipeline = nullptr;
	};
}