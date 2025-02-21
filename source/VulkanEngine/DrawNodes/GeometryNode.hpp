#pragma once

#include <DrawNodes/DrawNode.hpp>
#include <DrawNodes/Utils/Pipeline.hpp>
#include <DrawNodes/Utils/BindingTable.hpp>

/* 
	Draw scene geometry into G-Buffers
*/

namespace NVulkanEngine
{
	class CGeometryNode : public CDrawNode
	{
	public:
		CGeometryNode() = default;
		~CGeometryNode() = default;

		virtual void Init(CGraphicsContext* context, SGraphicsManagers* managers)  override;
		virtual void Draw(CGraphicsContext* context, SGraphicsManagers* managers, VkCommandBuffer commandBuffer) override;
		virtual void Cleanup(CGraphicsContext* context) override;

	private:
		void UpdateGeometryBuffers(CGraphicsContext* context, SGraphicsManagers* managers);

		// Pipeline (shader binding is done in model)
		CPipeline* m_GeometryPipeline = nullptr;
	} ;
}