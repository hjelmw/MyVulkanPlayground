#pragma once

#include <vulkan/vulkan.h>

#include <DrawPasses/DrawPass.hpp>
#include <DrawPasses/Pipeline.hpp>
#include <Managers/Model.hpp>

#include <vector>


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

		static glm::mat4 GetSphereMatrix() { return s_SphereMatrix; };

	private:
		void UpdateGeometryBuffers(CGraphicsContext* context, SGraphicsManagers* managers);

		// Pipeline
		CPipeline*					  m_GeometryPipeline    = nullptr;
		
		float                         m_RotationDegrees = 0.0f;
		glm::vec3                     m_LightPosition   = glm::vec3(0.0f, 1000.0f, 0.0f);

		static glm::mat4              s_SphereMatrix;
	} ;
}