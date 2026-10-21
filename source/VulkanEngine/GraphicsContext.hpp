#pragma once

#include <vulkan/vulkan.h>
#include "GLFW/glfw3.h"
/*
	Stores a bunch of useful members for the draw nodes. Might need to prune this a bit later
*/

namespace NVulkanEngine
{
	static uint32_t g_DisplayWidth      = 1920;
	static uint32_t g_DisplayHeight     = 1080;
	static const uint16_t g_MaxFramesInFlight = 2;

	struct SInternalContext
	{
		VkInstance        m_VulkanInstance                 = VK_NULL_HANDLE;
		VkSurfaceKHR      m_VulkanSurface                  = VK_NULL_HANDLE;
		GLFWwindow*       m_GLFWWindow                     = nullptr;
		VkDevice          m_VulkanDevice                   = VK_NULL_HANDLE;
		VkPhysicalDevice  m_PhysicalDevice                 = VK_NULL_HANDLE;
		VkCommandPool     m_CommandPool                    = VK_NULL_HANDLE;
		VkQueue           m_GraphicsQueue                  = VK_NULL_HANDLE;
		VkQueue           m_PresentQueue                   = VK_NULL_HANDLE;
		VkSampler         m_LinearClampSampler             = VK_NULL_HANDLE;
		VkSampler         m_LinearRepeatSampler            = VK_NULL_HANDLE;
		VkExtent2D        m_RenderResolution               = { 0,0 };
		float             m_DeltaTime                      = 0.0f;
		uint32_t          m_FrameIndex                     = 0;
		uint32_t          m_SwapchainImageIndex            = 0;
	};

	// Handed to draw nodes
	struct SRuntimeContext
	{
		VkInstance        m_VulkanInstance                 = VK_NULL_HANDLE;
		VkPhysicalDevice  m_PhysicalDevice                 = VK_NULL_HANDLE;
		VkDevice          m_VulkanDevice                   = VK_NULL_HANDLE;
		VkCommandPool     m_CommandPool                    = VK_NULL_HANDLE;
		VkQueue           m_GraphicsQueue                  = VK_NULL_HANDLE;
		VkSampler         m_LinearClampSampler             = VK_NULL_HANDLE;
		VkSampler         m_LinearRepeatSampler            = VK_NULL_HANDLE;
		VkExtent2D        m_RenderResolution               = { 0,0 };
		float             m_DeltaTime                      = 0.0f;
		uint32_t          m_FrameIndex                     = 0;
		uint32_t          m_SwapchainImageIndex            = 0;
	};
};
