#pragma once

#include <vulkan/vulkan.h>
#include "GLFW/glfw3.h"

namespace NVulkanEngine
{
	static const uint32_t g_DisplayWidth      = 1920;
	static const uint32_t g_DisplayHeight     = 1080;
	static const uint16_t g_MaxFramesInFlight = 2;

	//Singleton class
	class CGraphicsContext
	{
	public:
		CGraphicsContext();
		~CGraphicsContext();
		
		void CreateContext(
			VkInstance       instance,
			VkSurfaceKHR     surface,
			GLFWwindow*      window,
			VkPhysicalDevice physicalDevice,
			VkDevice         device,
			VkCommandPool    commandPool,
			VkQueue          graphicsQueue,
			VkQueue          presentQueue,
			VkSampler        sampler,
			VkExtent2D       renderResolution);

		const VkInstance       GetVulkanInstance()      { return m_VulkanInstance; }
		const VkSurfaceKHR     GetVulkanSurface()       { return m_VulkanSurface; }
		GLFWwindow*            GetGLFWWindow()          { return m_GLFWWindow; }
		const VkDevice         GetLogicalDevice()       { return m_VulkanDevice; }
		const VkPhysicalDevice GetPhysicalDevice()      { return m_PhysicalDevice; }
		const VkCommandPool    GetCommandPool()         { return m_CommandPool; }
		const VkQueue          GetGraphicsQueue()       { return m_GraphicsQueue; }
		const VkQueue          GetPresentQueue()        { return m_PresentQueue; }
		const VkSampler        GetLinearClampSampler()  { return m_LinearClampSampler; }
		const VkExtent2D       GetRenderResolution()    { return m_RenderResolution; }
		const float            GetDeltaTime()           { return m_DeltaTime; }
		const uint32_t         GetFrameIndex()          { return m_FrameIndex; }
		const uint32_t         GetSwapchainImageIndex() { return m_SwapchainImageIndex; }
		
		void SetDeltaTime(float deltaTime);
		void SetFrameIndex(uint32_t frameIndex);
		void SetSwapchainImageIndex(uint32_t imageIndex);
		void SetRenderResolution(const VkExtent2D resolution);

	private:
		VkInstance        m_VulkanInstance                 = VK_NULL_HANDLE;
		VkSurfaceKHR      m_VulkanSurface                  = VK_NULL_HANDLE;
		GLFWwindow*       m_GLFWWindow                     = nullptr;
		VkDevice          m_VulkanDevice                   = VK_NULL_HANDLE;
		VkPhysicalDevice  m_PhysicalDevice                 = VK_NULL_HANDLE;
		VkCommandPool     m_CommandPool                    = VK_NULL_HANDLE;
		VkQueue           m_GraphicsQueue                  = VK_NULL_HANDLE;
		VkQueue           m_PresentQueue                   = VK_NULL_HANDLE;
		VkSampler         m_LinearClampSampler             = VK_NULL_HANDLE;
		VkExtent2D        m_RenderResolution               = { 0,0 };
		float             m_DeltaTime                      = 0.0f;
		uint32_t          m_FrameIndex                     = 0;
		uint32_t          m_SwapchainImageIndex            = 0;
	};
};
