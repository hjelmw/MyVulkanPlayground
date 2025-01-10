#pragma once

#include <GraphicsContext.hpp>
#include <VulkanGraphicsEngineUtils.hpp>

#include <Managers/InputManager.hpp>
#include <Managers/ModelManager.hpp>
#include <Managers/AttachmentManager.hpp>

#include <Swapchain.hpp>

namespace NVulkanEngine
{
	enum class EDrawPasses : uint32_t
	{
		Geometry     = 0,
		Shadows      = 1,
		Terrain      = 2,
		Skybox       = 3,
		Lighting     = 4,
		Count
	};

	struct SGraphicsContext
	{
		VkDevice          m_VulkanDevice        = VK_NULL_HANDLE;
		VkExtent2D        m_RenderResolution    = { 0,0 };
		float             m_DeltaTime           = 0.0f;
		uint32_t          m_FrameIndex          = 0;
		uint32_t          m_SwapchainImageIndex = 0;
	};

	struct SGraphicsManagers
	{
		CInputManager*      m_InputManager      = nullptr;
		CModelManager*      m_Modelmanager      = nullptr;
		CAttachmentManager* m_AttachmentManager = nullptr;

	};

	class CDrawPass
	{
	public:
		virtual void InitPass(CGraphicsContext* context, SGraphicsManagers* managers);
		virtual void DrawPass(CGraphicsContext* context, SGraphicsManagers* managers, VkCommandBuffer commandBuffer);
		virtual void CleanupPass(CGraphicsContext* context);

	protected:
		void GenerateMipmaps(
			CGraphicsContext* context, 
			VkImage           image, 
			VkFormat          imageFormat, 
			int32_t           texWidth, 
			int32_t           texHeight, 
			uint32_t          mipLevels);

		// Begin rendering with attachments
		void BeginRendering(
			const std::string              markerName,
			CGraphicsContext*              context,
			VkCommandBuffer                commandBuffer,
			std::vector<SRenderAttachment> attachmentInfos);

		void EndRendering(CGraphicsContext* context, VkCommandBuffer commandBuffer);
	};
};
