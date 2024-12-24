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

		virtual void InitPass(CGraphicsContext* context)  override;
		virtual void Draw(CGraphicsContext* context, VkCommandBuffer commandBuffer) override;
		virtual void CleanupPass(CGraphicsContext* context) override;

		static SRenderAttachment GetGBufferAttachment(ERenderAttachments index) { return s_GeometryAttachments[(uint32_t)index]; }
		static glm::mat4 GetSphereMatrix() { return s_SphereMatrix; };

	private:
		void UpdateGeometryBuffers(CGraphicsContext* context);

		// Geometry pass image attachments (gbuffers) needs to be static since deferred lighting pass needs to sample them
		inline static std::vector<SRenderAttachment> s_GeometryAttachments;

		// Geometry buffers
		VkBuffer					  m_GeometryBufferSphere        = VK_NULL_HANDLE;
		VkDeviceMemory				  m_GeometryBufferMemorySphere  = VK_NULL_HANDLE;
		VkBuffer					  m_GeometryBufferFloor         = VK_NULL_HANDLE;
		VkDeviceMemory				  m_GeometryBufferMemoryFloor   = VK_NULL_HANDLE;
		VkBuffer					  m_GeometryBufferShip          = VK_NULL_HANDLE;
		VkDeviceMemory				  m_GeometryBufferMemoryShip    = VK_NULL_HANDLE;

		std::vector<VkDescriptorSet> m_DescriptorSetsShip   = { VK_NULL_HANDLE };
		std::vector<VkDescriptorSet> m_DescriptorSetFloor   = { VK_NULL_HANDLE };
		std::vector<VkDescriptorSet> m_DescriptorSetsSphere = { VK_NULL_HANDLE };

		// Pipeline
		CPipeline*					  m_GeometryPipeline    = nullptr;

		VkSampler                     m_GeometrySampler     = VK_NULL_HANDLE;
		
		float                         m_RotationDegrees = 0.0f;
		glm::vec3                     m_LightPosition   = glm::vec3(0.0f, 0.0f, 0.0f);

		static glm::mat4              s_SphereMatrix;
	} ;
}