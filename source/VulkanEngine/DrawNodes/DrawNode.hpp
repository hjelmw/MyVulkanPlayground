#pragma once

#include <GraphicsContext.hpp>
#include <VulkanGraphicsEngineUtils.hpp>

#include <Managers/InputManager.hpp>
#include <Managers/ModelManager.hpp>
#include <Managers/LightManager.hpp>
#include <Managers/DebugManager.hpp>
#include <Managers/ResourceManager.hpp>

/*
	Draw nodes. Used for drawing everything in this engine.
	See all the other *Node.hpp files
*/

namespace NVulkanEngine
{
	enum class EDrawNodes : uint32_t
	{
		Geometry     = 0,
		Terrain      = 1,
		Shadows      = 2,
		Skybox       = 3,
		Lighting     = 4,
		PostProcess  = 5,
		Debug        = 6,
		Count
	};

	struct SGraphicsManagers
	{
		CInputManager*      m_InputManager      = nullptr;
		CModelManager*      m_Modelmanager      = nullptr;
		CLightManager*      m_LightManager      = nullptr;
		CDebugManager*      m_DebugManager      = nullptr;
		CResourceManager*   m_ResourceManager   = nullptr;
	};

	class CDrawNode
	{
	public:
		virtual void Init(SRuntimeContext* context, SGraphicsManagers* managers);
		virtual void UpdateBeforeDraw(VkDevice logicalDevice, SGraphicsManagers* managers);
		virtual void Draw(SRuntimeContext* context, SGraphicsManagers* managers, VkCommandBuffer commandBuffer);
		virtual void Cleanup(SRuntimeContext* context);

	protected:
		void GenerateMipmaps(
			SRuntimeContext* context, 
			VkImage           image, 
			VkFormat          imageFormat, 
			int32_t           texWidth, 
			int32_t           texHeight, 
			uint32_t          mipLevels);

		// Begin rendering with attachments
		void BeginRendering(
			const std::string              markerName,
			SRuntimeContext*              context,
			VkCommandBuffer                commandBuffer,
			std::vector<SRenderResource> attachmentInfos,
			VkExtent2D                   renderExtentOverride = { 0, 0 }); // Defaults to the context render resolution

		void EndRendering(SRuntimeContext* context, VkCommandBuffer commandBuffer);
	};
};
