#pragma once

// 3rd party
#include <vulkan/vulkan.h>
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <GLFW/glfw3.h>

#include "Swapchain.hpp"
#include "DrawPass.hpp"
#include "ModelManager.hpp"


#include "GraphicsContext.hpp"

namespace NVulkanEngine
{
	class CVulkanGraphicsEngine
	{
	public:
		CVulkanGraphicsEngine()  = default;
		~CVulkanGraphicsEngine() = default;

		void Init();

		void AddModel(const std::string& filepath);
		void SetModelPosition(float x, float y, float z);
		void SetModelRotation(float x, float y, float z);
		void SetModelScaling(float x, float y, float z);

		void DrawFrame();
		void Cleanup();

	private:
		void InitWindow();
		void InitVulkan();
		void InitSwapchain();
		void InitCamera();
		void CreateGraphicsContext();
		void InitDrawPasses();
		void CreateModels();

		void CreateVulkanInstance();
		void SetupDebugMessenger();
		void CreateVulkanSurface();
		void SelectPhysicalDevice();
		void CreateLogicalDevice();
		void CreateCommandPool();
		void CreateCommandBuffers();
		void CreateSyncObjects();

		void CleanupSwapchain();
		void CleanupDrawPasses();
		void CleanupVulkan();
		void CleanupWindow();

		bool CheckDeviceExtensionSupport(VkPhysicalDevice device, const std::vector<const char*> deviceExtensions);
		bool IsDeviceSuitable(VkPhysicalDevice device, VkSurfaceKHR surface, const std::vector<const char*> deviceExtensions);

		void     PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
		VkResult CreateDebugUtilsMessengerEXT(const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger);
		VkResult DestroyDebugUtilsMessengerEXT(VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator);

		void     RecordDrawPasses(VkCommandBuffer commandBuffer);

		CGraphicsContext*	                m_Context                  = nullptr;

		// Draw passes specifies render order
		std::vector<CDrawPass*>             m_DrawPasses               = {};

		CModelManager*                      m_ModelManager             = {};

		/* Vulkan Primitives */
		// Device
		VkInstance			                m_VulkanInstance           = VK_NULL_HANDLE;
		VkPhysicalDevice	                m_PhysicalDevice           = VK_NULL_HANDLE;
		VkDevice			                m_VulkanDevice             = VK_NULL_HANDLE;
		VkSurfaceKHR		                m_VulkanSurface            = VK_NULL_HANDLE;

		// Queues
		VkQueue				                m_GraphicsQueue            = VK_NULL_HANDLE;
		VkQueue				                m_PresentQueue             = VK_NULL_HANDLE;

		// CommandBuffer
		VkCommandPool                       m_CommandPool              = VK_NULL_HANDLE;
		std::vector<VkCommandBuffer>        m_CommandBuffers           = {};

		// Swapchain extent
		VkExtent2D					        m_RenderResolution         = { 0, 0 };

		// Frame Index
		uint32_t                            m_FrameIndex               = 0;

		// Synchronization
		std::vector<VkSemaphore>            m_ImageAvailableSemaphores = {};
		std::vector<VkSemaphore>            m_RenderFinishedSemaphores = {};
		std::vector<VkFence>                m_InFlightFences           = {};

		// Misc
		VkDebugUtilsMessengerEXT            m_DebugMessenger           = VK_NULL_HANDLE;
		GLFWwindow*					        m_Window                   = nullptr;


	};
}