#pragma once

#include <Draw/DrawNodes/DrawNode.hpp>

/*
	Compute post processing on the scene color
*/

namespace NVulkanEngine
{
	class CPipeline;
	class CBindingTable;

	class CPostProcessNode : public CDrawNode
	{
	public:
		CPostProcessNode() = default;
		~CPostProcessNode() = default;

		virtual void Init(SRuntimeContext* context, SGraphicsManagers* managers) override;
		virtual void Draw(SRuntimeContext* context, SGraphicsManagers* managers, VkCommandBuffer commandBuffer) override;
		virtual void Cleanup(SRuntimeContext* context) override;

	private:
		// Must match local_size_x/y in postprocess.comp
		static constexpr uint32_t s_WorkGroupSize = 8;

		// Pipeline & shader binding
		CBindingTable* m_PostProcessTable    = nullptr;
		CPipeline*     m_PostProcessPipeline = nullptr;
	};
}
