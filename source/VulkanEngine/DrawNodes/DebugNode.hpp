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
		virtual void UpdateBeforeDraw(VkDevice logicalDevice, SGraphicsManagers* managers) override;
		virtual void Draw(CGraphicsContext* context, SGraphicsManagers* managers, VkCommandBuffer commandBuffer) override;
		virtual void Cleanup(CGraphicsContext* context) override;

	private:

		// Shadow Uniform Buffer
		CPipeline* m_DebugPipeline  = nullptr;
	};
}