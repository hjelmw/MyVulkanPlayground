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

		virtual void InitPass(CGraphicsContext* context, SGraphicsManagers* managers)  override;
		virtual void Draw(CGraphicsContext* context, SGraphicsManagers* managers, VkCommandBuffer commandBuffer) override;
		virtual void CleanupPass(CGraphicsContext* context) override;

		static glm::mat4 GetLightMatrix() { return s_LightMatrix; };

	private:
		void UpdateShadowBuffers(CGraphicsContext* context, SGraphicsManagers* managers);

		std::vector<SDescriptorSets> m_DescriptorSetsShadow = { };

		// Shadow Uniform Buffer
		VkBuffer					  m_ShadowBuffer       = VK_NULL_HANDLE;
		VkDeviceMemory				  m_ShadowBufferMemory = VK_NULL_HANDLE;

		static glm::mat4              s_LightMatrix;

		// Pipeline
		CPipeline* m_ShadowPipeline = nullptr;
	};
}