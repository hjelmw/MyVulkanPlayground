#pragma once

#include <vector>

#include "DrawNodes/DrawNode.hpp"

/*
	Holds a sorted list of draw nodes and uses it to decide order of execution and transition of resources
*/

namespace NVulkanEngine
{
	class CDrawGraph
	{
	public:
		void Add(CDrawNode* node);
		void Compile();

		void Init(SRuntimeContext* context, SGraphicsManagers* managers);
		void UpdateBeforeDraw(VkDevice logicalDevice, SGraphicsManagers* managers);
		void Draw(SRuntimeContext* context, SGraphicsManagers* managers, VkCommandBuffer commandBuffer);

		void Cleanup(SRuntimeContext* context);

	private:
		struct SResourceTransition
		{
			uint32_t           m_ResourceId = UINT32_MAX;
			VkImageLayout      m_NewLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
			VkAttachmentLoadOp m_LoadOp     = VK_ATTACHMENT_LOAD_OP_LOAD;
		};

		struct SCompiledNode
		{
			CDrawNode*                       m_Node = nullptr;
			std::vector<SResourceTransition> m_Transitions;
		};

		std::vector<CDrawNode*>     m_PendingNodes;
		std::vector<SCompiledNode>  m_ExecutionOrder;
		bool                        m_IsCompiled = false;
	};
};
