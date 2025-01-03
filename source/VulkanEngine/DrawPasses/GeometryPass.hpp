#pragma once

#include <DrawPasses/DrawPass.hpp>
#include <DrawPasses/Pipeline.hpp>
#include <DrawPasses/BindingTable.hpp>

namespace NVulkanEngine
{
	class CGeometryPass : public CDrawPass
	{
	public:
		CGeometryPass() = default;
		~CGeometryPass() = default;

		virtual void InitPass(CGraphicsContext* context, SGraphicsManagers* managers)  override;
		virtual void Draw(CGraphicsContext* context, SGraphicsManagers* managers, VkCommandBuffer commandBuffer) override;
		virtual void CleanupPass(CGraphicsContext* context) override;

	private:
		void UpdateGeometryBuffers(CGraphicsContext* context, SGraphicsManagers* managers);

		// Pipeline (shader binding is done in model)
		CPipeline* m_GeometryPipeline = nullptr;
	} ;
}