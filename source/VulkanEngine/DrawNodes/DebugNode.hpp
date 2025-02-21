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
		void UpdateDebugBuffers(CGraphicsContext* context, SGraphicsManagers* managers);

		std::vector<SDescriptorSets> m_DescriptorSetsDebug = { };

		VkBuffer					  m_VertexBuffer       = VK_NULL_HANDLE;
		VkDeviceMemory				  m_VertexBufferMemory = VK_NULL_HANDLE;

		// Shadow Uniform Buffer
		VkBuffer					  m_DebugUniformBuffer       = VK_NULL_HANDLE;
		VkDeviceMemory				  m_DebuguniformBufferMemory = VK_NULL_HANDLE;

		static glm::mat4              s_LightMatrix;

		CPipeline* m_DebugPipeline  = nullptr;
		CBindingTable* m_DebugTable = nullptr;
	};
}