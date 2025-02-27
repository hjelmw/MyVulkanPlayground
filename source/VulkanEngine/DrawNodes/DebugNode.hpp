#pragma once

#include <vulkan/vulkan.h>

#include <DrawNodes/DrawNode.hpp>
#include <DrawNodes/Utils/Pipeline.hpp>

/*
	Draw debug lines
*/

namespace NVulkanEngine
{
	class CDebugNode : public CDrawNode
	{
	public:
		CDebugNode() = default;
		~CDebugNode() = default;

		virtual void Init(CGraphicsContext* context, SGraphicsManagers* managers)  override;
		virtual void Draw(CGraphicsContext* context, SGraphicsManagers* managers, VkCommandBuffer commandBuffer) override;
		virtual void Cleanup(CGraphicsContext* context) override;

	private:
		void UpdateDebugUniformBuffer(CGraphicsContext* context, SGraphicsManagers* managers);

		// Shadow Uniform Buffer
		VkBuffer					  m_DebugUniformBuffer       = VK_NULL_HANDLE;
		VkDeviceMemory				  m_DebuguniformBufferMemory = VK_NULL_HANDLE;

		CPipeline* m_DebugPipeline  = nullptr;
		CBindingTable* m_DebugTable = nullptr;
	};
}