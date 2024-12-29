#include "GraphicsContext.hpp"

namespace NVulkanEngine
{
	CGraphicsContext::CGraphicsContext()
	{
	}
	CGraphicsContext::~CGraphicsContext()
	{
	}

	void CGraphicsContext::CreateContext(
		VkInstance       instance,
		VkSurfaceKHR     surface,
		GLFWwindow*      window,
		VkPhysicalDevice physicalDevice,
		VkDevice         device,
		VkCommandPool    commandPool,
		VkQueue          graphicsQueue,
		VkQueue          presentQueue,
		VkSampler        linearClampSampler,
		VkSampler        linearRepeatSampler,
		VkExtent2D       renderResolution)
	{
		m_VulkanInstance      = instance;
		m_VulkanSurface       = surface;
		m_GLFWWindow          = window;
		m_PhysicalDevice      = physicalDevice;
		m_VulkanDevice        = device;
		m_CommandPool         = commandPool;
		m_GraphicsQueue       = graphicsQueue;
		m_PresentQueue        = presentQueue;
		m_LinearClampSampler  = linearClampSampler;
		m_LinearRepeatSampler = linearRepeatSampler;
		m_RenderResolution    = renderResolution;
	}

	void CGraphicsContext::SetDeltaTime(float deltaTime) 
	{ 
		m_DeltaTime = deltaTime; 
	}
	void CGraphicsContext::SetFrameIndex(uint32_t frameIndex) 
	{ 
		m_FrameIndex = frameIndex; 
	}
	void CGraphicsContext::SetSwapchainImageIndex(uint32_t imageIndex) 
	{ 
		m_SwapchainImageIndex = imageIndex; 
	}

	void CGraphicsContext::SetRenderResolution(VkExtent2D resolution)
	{
		m_RenderResolution = resolution;
	}
};