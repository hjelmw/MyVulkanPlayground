#pragma once

#include <vulkan/vulkan.hpp>

#include <GraphicsContext.hpp>
#include <VulkanGraphicsEngineUtils.hpp>

#include <Swapchain.hpp>

#include <Managers/InputManager.hpp>
#include <Managers/ModelManager.hpp>
#include <Managers/AttachmentManager.hpp>

#include <glm/glm.hpp>
#include <array>

// Remember to store in this order
enum class ERenderAttachments : uint32_t
{
	Positions          = 0,
	Normals            = 1,
	Albedo             = 2,
	Depth              = 3,
	ShadowMap          = 4,
	SceneColor         = 5,
	AtmosphericsSkyBox = 6,
	Count              = 7
};

		

namespace NVulkanEngine
{
	enum class EDrawPasses : uint32_t
	{
		Geometry     = 0,
		Shadows      = 1,
		Atmospherics = 2,
		Lighting     = 3,
		Count
	};

	struct SGraphicsContext
	{
		VkInstance        m_VulkanInstance      = VK_NULL_HANDLE;
		VkSurfaceKHR      m_VulkanSurface       = VK_NULL_HANDLE;
		VkDevice          m_VulkanDevice        = VK_NULL_HANDLE;
		VkPhysicalDevice  m_PhysicalDevice      = VK_NULL_HANDLE;
		VkCommandPool     m_CommandPool         = VK_NULL_HANDLE;
		VkQueue           m_GraphicsQueue       = VK_NULL_HANDLE;
		VkQueue           m_PresentQueue        = VK_NULL_HANDLE;
		GLFWwindow*       m_GLFWWindow          = nullptr;
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
		virtual void Draw(CGraphicsContext* context, SGraphicsManagers* managers, VkCommandBuffer commandBuffer);
		virtual void CleanupPass(CGraphicsContext* context);

	protected:
		// Creates the descriptor pool from which descriptor sets can be allocated from
		void AllocateDescriptorPool(
			CGraphicsContext* context,
			uint32_t          numDescriptorSets,
			uint32_t          descriptorImageCount,
			uint32_t          descriptorBufferCount);

		// Update descriptor set layout with a write descriptor set (vector of image views)
		void UpdateDescriptorSets(
			CGraphicsContext*                 context, 
			std::vector<VkDescriptorSet>      descriptorSets,
			std::vector<VkWriteDescriptorSet> writeDescriptorSets);

		// Build vulkan descriptor image & buffer structs
		VkDescriptorImageInfo  CreateDescriptorImageInfo(VkSampler sampler, VkImageView imageView, VkImageLayout imageLayout);
		VkDescriptorBufferInfo CreateDescriptorBufferInfo(VkBuffer uniformBuffer, uint32_t range);

		// Create write descriptors at the specified binding and slot
		VkWriteDescriptorSet CreateWriteDescriptorImage(
			CGraphicsContext*      context, 
			VkDescriptorSet*       descriptorSets, 
			VkDescriptorType       descriptorType, 
			uint32_t               descriptorSlot, 
			VkDescriptorImageInfo* descriptorImageInfo);

		// Create write descriptor for buffer at the specified binding and slot
		VkWriteDescriptorSet CreateWriteDescriptorBuffer(
			CGraphicsContext*       context, 
			VkDescriptorSet*        descriptorSets, 
			VkDescriptorType        descriptorType, 
			uint32_t                descriptorSlot, 
			VkDescriptorBufferInfo* descriptorBufferInfo);

		
		void GenerateMipmaps(
			CGraphicsContext* context, 
			VkImage           image, 
			VkFormat          imageFormat, 
			int32_t           texWidth, 
			int32_t           texHeight, 
			uint32_t          mipLevels);

		SRenderAttachment GetSwapchainAttachment(CGraphicsContext* context);

		// Begin rendering with attachments
		void BeginRendering(
			CGraphicsContext*             context,
			VkCommandBuffer               commandBuffer,
			std::vector<SRenderAttachment> attachmentInfos);

		void EndRendering(VkCommandBuffer commandBuffer);

		void PushConstants(VkCommandBuffer commandBuffer, VkShaderStageFlags shaderStage, void* data, size_t constantsSize);

		// Copies buffer into image
		void CopyBufferToImage(
			CGraphicsContext* context, 
			VkBuffer buffer, 
			VkImage image, 
			uint32_t width, 
			uint32_t height);

		std::vector<SRenderAttachment> m_Attachments = {};

		VkDescriptorSetLayout        m_DescriptorSetLayout   = VK_NULL_HANDLE;
		VkPipelineLayout             m_PipelineLayout        = VK_NULL_HANDLE;
		VkDescriptorPool             m_DescriptorPool        = VK_NULL_HANDLE;
	};
};
