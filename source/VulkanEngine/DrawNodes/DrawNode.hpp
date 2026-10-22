#pragma once

#include <GraphicsContext.hpp>
#include <VulkanGraphicsEngineUtils.hpp>

#include <Managers/InputManager.hpp>
#include <Managers/ModelManager.hpp>
#include <Managers/LightManager.hpp>
#include <Managers/DebugManager.hpp>
#include <Managers/ResourceManager.hpp>
#include <Managers/RenderResources.hpp>

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

	enum class EResourceAccess : uint8_t
	{
		Read,
		Write,
		ReadWrite
	};

	// What a node declared about one resource. The tag type is erased, only its id is kept
	struct SResourceUse
	{
		uint32_t        m_ResourceId = UINT32_MAX;
		EResourceAccess m_Access     = EResourceAccess::Read;
		VkImageLayout   m_Layout     = VK_IMAGE_LAYOUT_UNDEFINED;
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

		template<class TRenderResource>
		void Read(VkImageLayout layout)
		{
			m_ResourceUses.push_back({ ResourceId<TRenderResource>(), EResourceAccess::Read, layout });
		}

		template<class TRenderResource>
		void Write(VkImageLayout layout)
		{
			m_ResourceUses.push_back({ ResourceId<TRenderResource>(), EResourceAccess::Write, layout });
		}

		
		template<class TRenderResource>
		void ReadWrite(VkImageLayout layout)
		{
			m_ResourceUses.push_back({ ResourceId<TRenderResource>(), EResourceAccess::ReadWrite, layout });
		}

		const std::vector<SResourceUse>& GetResourceUses() const { return m_ResourceUses; }

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

	private:
		std::vector<SResourceUse> m_ResourceUses;
	};
};
