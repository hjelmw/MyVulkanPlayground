#pragma once

#include <VulkanGraphicsEngineUtils.hpp>

/*
	The swapchain. I.e the queue of images that we draw into for presenting
*/

namespace NVulkanEngine
{
	class CSwapchain
	{
	public:
		CSwapchain()  = default;
		~CSwapchain() = default;

		// Creates the swapchain
		void                Create(CGraphicsContext* context);

		// Acquires the swapchain image at the provided index
		VkResult            AcquireSwapchainImageIndex(CGraphicsContext* context, VkSemaphore imageAvailableSemaphore, uint32_t& imageIndex);

		// Gets the current frame swapchain image. Needed for transitions
		VkImage             GetSwapchainImage(uint32_t imageIndex);

		// Gets the current frame view that can be rendered into
		VkImageView         GetSwapchainImageView(uint32_t imageIndex);

		// Return swapchain format (currently SRGB)
		VkFormat            GetSwapchainFormat() { return m_SwapchainImageFormat; }

		// Returns the swapchain extent (i.e resolution)
		VkExtent2D	        GetExtent() { return m_SwapchainExtent; };

		uint32_t            GetMinImageCount() { return m_ImageCount; };

		// Destroys and initializes the swap chain again
		void                Recreate(CGraphicsContext* context);

		// Destroy the swapchain
		void                Cleanup(CGraphicsContext* context);

		// Presents the finished rendered swapchain image to the present queue
		void                Present(CGraphicsContext* context, VkSemaphore* signalSemaphore, uint32_t imageIndex);

	private:
		VkExtent2D         ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& surfaceCapabilities, GLFWwindow* window);
		VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
		VkPresentModeKHR   ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
		void               CreateSwapChainImageViews(VkDevice vulkanDevice);

		struct SSwapChainSupportDetails
		{
			VkSurfaceCapabilitiesKHR capabilities = {};
			std::vector<VkSurfaceFormatKHR> formats = {};
			std::vector<VkPresentModeKHR> presentModes = {};
		};
		SSwapChainSupportDetails QuerySwapChainSupport(VkPhysicalDevice physicaldevice, VkSurfaceKHR vulkanSurface);

		VkSwapchainKHR           m_VulkanSwapchain      =   VK_NULL_HANDLE;

		std::vector<VkImage>     m_SwapchainImages      = { VK_NULL_HANDLE };
	    std::vector<VkImageView> m_SwapchainImageViews  = { VK_NULL_HANDLE };
		VkFormat                 m_SwapchainImageFormat = VK_FORMAT_UNDEFINED;

		VkExtent2D               m_SwapchainExtent      = { 0, 0 };

		uint32_t                 m_ImageCount = 0;
	};
}