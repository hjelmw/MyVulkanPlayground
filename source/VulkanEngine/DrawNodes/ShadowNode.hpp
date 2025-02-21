#pragma once

#include <vulkan/vulkan.h>

#include <DrawNodes/DrawNode.hpp>
#include <DrawNodes/Utils/Pipeline.hpp>

/* 
	Draw geometry into shadow map depth buffer
*/

namespace NVulkanEngine
{
	class CShadowNode : public CDrawNode
	{
	public:
		CShadowNode() = default;
		~CShadowNode() = default;

		virtual void Init(CGraphicsContext* context, SGraphicsManagers* managers)  override;
		virtual void Draw(CGraphicsContext* context, SGraphicsManagers* managers, VkCommandBuffer commandBuffer) override;
		virtual void Cleanup(CGraphicsContext* context) override;

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