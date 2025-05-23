#pragma once

#include <GraphicsContext.hpp>
#include <VulkanGraphicsEngineUtils.hpp>

#include <Managers/InputManager.hpp>
#include <Managers/ModelManager.hpp>
#include <Managers/LightManager.hpp>
#include <Managers/DebugManager.hpp>
#include <Managers/ResourceManager.hpp>
#include <Managers/PipelineManager.hpp>

/*
	Draw nodes. Used for drawing everything in this engine.
	See all the other *Node.hpp files
*/

namespace NVulkanEngine
{
	enum class EDrawNodes : uint32_t
	{
		Geometry     = 0,
		Shadows      = 1,
		Terrain      = 2,
		Skybox       = 3,
		Lighting     = 4,
		Debug        = 5,
		Count
	};

	struct SGraphicsContext
	{
		VkInstance        m_VulkanInstance      = VK_NULL_HANDLE;
		VkDevice          m_VulkanDevice        = VK_NULL_HANDLE;
		VkExtent2D        m_RenderResolution    = { 0,0 };
		uint32_t          m_FrameIndex          = 0;
	};

	struct SGraphicsManagers
	{
		CInputManager*      m_InputManager      = nullptr;
		CModelManager*      m_Modelmanager      = nullptr;
		CLightManager*      m_LightManager      = nullptr;
		CDebugManager*      m_DebugManager      = nullptr;
		CPipelineManager*   m_PipelineManager   = nullptr;
		CResourceManager*   m_ResourceManager   = nullptr;
	};

	class CDrawNode
	{
	public:
		virtual void Init(CGraphicsContext* context, SGraphicsManagers* managers);
		virtual void UpdateBeforeDraw(VkDevice logicalDevice, SGraphicsManagers* managers);
		virtual void Draw(CGraphicsContext* context, SGraphicsManagers* managers, VkCommandBuffer commandBuffer);
		virtual void Cleanup(CGraphicsContext* context);

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
			std::vector<SRenderResource> attachmentInfos);

		void EndRendering(CGraphicsContext* context, VkCommandBuffer commandBuffer);
	};
};
