#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

#include <VulkanGraphicsEngineUtils.hpp>
#include <Swapchain.hpp>

namespace NVulkanEngine
{
	CSwapchain* CSwapchain::s_SwapchainInstance = nullptr;

	CSwapchain* CSwapchain::GetInstance()
	{
		if (!s_SwapchainInstance)
		{
			s_SwapchainInstance = new CSwapchain();
		}

		return s_SwapchainInstance;
	}

	void CSwapchain::CreateSwapchain(CGraphicsContext* context)
	{
		CSwapchain::SSwapChainSupportDetails swapChainSupport = QuerySwapChainSupport(context->GetPhysicalDevice(), context->GetVulkanSurface());

		VkSurfaceFormatKHR surfaceFormat = ChooseSwapSurfaceFormat(swapChainSupport.formats);
		VkPresentModeKHR presentMode = ChooseSwapPresentMode(swapChainSupport.presentModes);
		VkExtent2D extent = ChooseSwapExtent(swapChainSupport.capabilities, context->GetGLFWWindow());

		// Request one more image than the minium supported
		m_ImageCount = swapChainSupport.capabilities.minImageCount + 1;
		if (swapChainSupport.capabilities.maxImageCount > 0 && m_ImageCount > swapChainSupport.capabilities.maxImageCount)
		{
			m_ImageCount = swapChainSupport.capabilities.maxImageCount;
		}

		VkSwapchainCreateInfoKHR createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		createInfo.surface = context->GetVulkanSurface();
		createInfo.minImageCount = m_ImageCount;
		createInfo.imageFormat = surfaceFormat.format;
		createInfo.imageColorSpace = surfaceFormat.colorSpace;
		createInfo.imageExtent = extent;
		createInfo.imageArrayLayers = 1;
		createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

		SVulkanQueueFamilyIndices indices = FindVulkanQueueFamilies(context->GetPhysicalDevice(), context->GetVulkanSurface());
		uint32_t queueFamilyIndices[] = { indices.m_GraphicsFamily.value(), indices.m_PresentFamily.value() };

		if (indices.m_GraphicsFamily != indices.m_PresentFamily)
		{
			// Graphics & Presentation are in different queue families
			createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
			createInfo.queueFamilyIndexCount = sizeof(queueFamilyIndices) / sizeof(queueFamilyIndices[0]);
			createInfo.pQueueFamilyIndices = queueFamilyIndices;
		}
		else
		{
			// True for most hardware. Queue Family is the same for Graphics & Presentation queue
			createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
			createInfo.queueFamilyIndexCount = 0;     // Optional
			createInfo.pQueueFamilyIndices = nullptr; // Optional
		}

		createInfo.preTransform = swapChainSupport.capabilities.currentTransform;

		// This is a special case of blending between multiple windows which we don't currently need. It does not mean alpha blending is disabled in general. 
		createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		createInfo.presentMode = presentMode;

		// Clip obscured pixels
		createInfo.clipped = VK_TRUE;

		// If we recreate the swapchain, this needs to point to the old one.
		createInfo.oldSwapchain = VK_NULL_HANDLE;

		if (vkCreateSwapchainKHR(context->GetLogicalDevice(), &createInfo, nullptr, &m_VulkanSwapchain) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create swap chain!");
		}

		vkGetSwapchainImagesKHR(context->GetLogicalDevice(), m_VulkanSwapchain, &m_ImageCount, nullptr);
		m_SwapchainImages.resize(m_ImageCount);
		vkGetSwapchainImagesKHR(context->GetLogicalDevice(), m_VulkanSwapchain, &m_ImageCount, m_SwapchainImages.data());

		m_SwapchainImageFormat = surfaceFormat.format;
		m_SwapchainExtent = extent;

		CreateSwapChainImageViews(context->GetLogicalDevice());

#if defined(_DEBUG)
		std::cout << "Swapchain succesfully created!\n" << std::endl;
#endif
	}

	VkExtent2D CSwapchain::ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& surfaceCapabilities, GLFWwindow* window)
	{
		if (surfaceCapabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
		{
			return surfaceCapabilities.currentExtent;
		}
		else
		{
			int width, height;
			glfwGetWindowSize(window, &width, &height);

			VkExtent2D actualExtent =
			{
				static_cast<uint32_t>(width),
				static_cast<uint32_t>(height)
			};

			actualExtent.width = std::clamp(actualExtent.width, surfaceCapabilities.minImageExtent.width, surfaceCapabilities.maxImageExtent.width);
			actualExtent.height = std::clamp(actualExtent.height, surfaceCapabilities.minImageExtent.height, surfaceCapabilities.maxImageExtent.height);

			return actualExtent;
		}
	}

	CSwapchain::SSwapChainSupportDetails CSwapchain::QuerySwapChainSupport(VkPhysicalDevice physicaldevice, VkSurfaceKHR vulkanSurface)
	{
		CSwapchain::SSwapChainSupportDetails details;

		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicaldevice, vulkanSurface, &details.capabilities);

		uint32_t formatCount;
		vkGetPhysicalDeviceSurfaceFormatsKHR(physicaldevice, vulkanSurface, &formatCount, nullptr);
		if (formatCount != 0)
		{
			details.formats.resize(formatCount);
			vkGetPhysicalDeviceSurfaceFormatsKHR(physicaldevice, vulkanSurface, &formatCount, details.formats.data());
		}

		uint32_t presentModeCount;
		vkGetPhysicalDeviceSurfacePresentModesKHR(physicaldevice, vulkanSurface, &presentModeCount, nullptr);
		if (presentModeCount != 0)
		{
			details.presentModes.resize(presentModeCount);
			vkGetPhysicalDeviceSurfacePresentModesKHR(physicaldevice, vulkanSurface, &presentModeCount, details.presentModes.data());
		}

		return details;
	}

	VkSurfaceFormatKHR CSwapchain::ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
	{
		for (const auto& availableFormat : availableFormats)
		{
			if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
			{
				return availableFormat;
			}
		}

		return availableFormats[0];
	}

	VkPresentModeKHR CSwapchain::ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes)
	{

		//for (const auto& availablePresentMode : availablePresentModes)
		//{
		//	if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
		//	{
		//		return availablePresentMode;
		//	}
		//}

		return VK_PRESENT_MODE_FIFO_KHR;
	}


	void CSwapchain::CreateSwapChainImageViews(VkDevice vulkanDevice)
	{
		m_SwapchainImageViews.resize(m_SwapchainImages.size());

		for (size_t i = 0; i < m_SwapchainImageViews.size(); i++)
		{
			VkImageViewCreateInfo viewInfo{};
			viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			viewInfo.image = m_SwapchainImages[i];
			viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			viewInfo.format = m_SwapchainImageFormat;
			viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			viewInfo.subresourceRange.baseMipLevel = 0;
			viewInfo.subresourceRange.levelCount = 1;
			viewInfo.subresourceRange.baseArrayLayer = 0;
			viewInfo.subresourceRange.layerCount = 1;

			if (vkCreateImageView(vulkanDevice, &viewInfo, nullptr, &m_SwapchainImageViews[i]) != VK_SUCCESS)
			{
				throw std::runtime_error("failed to create swapchain image view!");
			}
		}
	}

	ESwapchainResult CSwapchain::AcquireSwapchainImageIndex(CGraphicsContext* context, VkSemaphore imageAvailableSemaphore, uint32_t& imageIndex)
	{
		VkResult result = vkAcquireNextImageKHR(context->GetLogicalDevice(), m_VulkanSwapchain, UINT64_MAX, imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);
		return result == VK_SUCCESS ? ESuccesful : EErrorFailure;
	}

	VkImage CSwapchain::GetSwapchainImage(uint32_t imageIndex)
	{
		return m_SwapchainImages[imageIndex];
	}

	VkImageView CSwapchain::GetSwapchainImageView(uint32_t imageIndex)
	{
		return m_SwapchainImageViews[imageIndex];
	}
	
	void CSwapchain::Present(CGraphicsContext* context, VkSemaphore* signalSemaphores, uint32_t imageIndex)
	{
		VkPresentInfoKHR presentInfo{};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = signalSemaphores;
		presentInfo.pResults = nullptr; // Optional

		VkSwapchainKHR swapchains[] = { m_VulkanSwapchain };
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = swapchains;
		presentInfo.pImageIndices = &imageIndex;

		VkResult result = vkQueuePresentKHR(context->GetPresentQueue(), &presentInfo);

		//return result == VK_SUCCESS ? ESuccesful : EErrorFailure;
	}

	void CSwapchain::CleanupSwapchain(CGraphicsContext* context)
	{
		for (size_t i = 0; i < m_SwapchainImageViews.size(); i++) {
			vkDestroyImageView(context->GetLogicalDevice(), m_SwapchainImageViews[i], nullptr);
		}

		vkDestroySwapchainKHR(context->GetLogicalDevice(), m_VulkanSwapchain, nullptr);

		delete s_SwapchainInstance;
	}
}
