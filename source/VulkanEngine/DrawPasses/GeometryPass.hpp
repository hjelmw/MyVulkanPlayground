#pragma once

#include <vulkan/vulkan.h>

#include "../DrawPass.hpp"
#include "../Pipeline.hpp"
#include "../Model.hpp"
#include "../Texture.hpp"

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

		static SImageAttachment GetGBufferAttachment(uint32_t index) { return s_GeometryAttachments[index]; }
		static glm::mat4 GetSphereMatrix() { return s_SphereMatrix; };

	private:
		void UpdateGeometryBuffers(CGraphicsContext* context);

		std::vector<VkDescriptorSetLayoutBinding> GetDescriptorSetLayoutBindings();

		// Geometry pass image attachments (gbuffers) needs to be static since deferred lighting pass needs to sample them
		inline static std::vector<SImageAttachment> s_GeometryAttachments;

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

		// Model vertices & indices
		CModel*                       m_ShipModel    = nullptr;
		CModel*                       m_SphereModel  = nullptr;
		CModel*                       m_FLoorModel   = nullptr;

		float                         m_RotationDegrees = 0.0f;
		static glm::mat4              s_SphereMatrix;

		CTexture*                     m_BoxTexture   = nullptr;
	} ;
}