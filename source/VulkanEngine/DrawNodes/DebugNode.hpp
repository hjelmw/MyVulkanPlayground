#pragma once

#include <vulkan/vulkan.h>

#include <DrawNodes/DrawNode.hpp>

/*
	Draw debug lines
*/

namespace NVulkanEngine
{
	class CPipeline;
	class CBindingTable;

	class CDebugNode : public CDrawNode
	{
	public:
		CDebugNode() = default;
		~CDebugNode() = default;

		virtual void Init(SRuntimeContext* context, SGraphicsManagers* managers)  override;
		virtual void UpdateBeforeDraw(VkDevice logicalDevice, SGraphicsManagers* managers) override;
		virtual void Draw(SRuntimeContext* context, SGraphicsManagers* managers, VkCommandBuffer commandBuffer) override;
		virtual void Cleanup(SRuntimeContext* context) override;

	private:

		// Shadow Uniform Buffer
		CPipeline*     m_DebugPipeline = nullptr;
		CBindingTable* m_DebugTable    = nullptr;
	};
}