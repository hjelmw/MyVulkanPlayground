#pragma once

#include <DrawNodes/DrawNode.hpp>

/* 
	Draw scene geometry into G-Buffers
*/

namespace NVulkanEngine
{
	class CPipeline;

	class CGeometryNode : public CDrawNode
	{
	public:
		CGeometryNode() = default;
		~CGeometryNode() = default;

		virtual void Init(SRuntimeContext* context, SGraphicsManagers* managers)  override;
		virtual void UpdateBeforeDraw(VkDevice logicalDevice, SGraphicsManagers* managers) override;
		virtual void Draw(SRuntimeContext* context, SGraphicsManagers* managers, VkCommandBuffer commandBuffer) override;
		virtual void Cleanup(SRuntimeContext* context) override;

	private:

		// Pipeline (shader binding is done in model)
		CPipeline* m_GeometryPipeline = nullptr;
	} ;
}