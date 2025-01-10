#include "VulkanGraphicsEngine.hpp"
#include "VulkanGraphicsEngineUtils.hpp"

#include <DrawPasses/GeometryPass.hpp>
#include <DrawPasses/LightingPass.hpp>
#include <DrawPasses/AtmosphericsPass.hpp>
#include <DrawPasses/ShadowPass.hpp>

#include <Managers/InputManager.hpp>
#include <Managers/ModelManager.hpp>

#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>

#include <cstdlib>
#include <vector>
#include <set>
#include <array>
#include <optional>
#include <chrono>
#include <thread>


static void check_vk_result(VkResult err)
{
	if (err == 0)
		return;
	fprintf(stderr, "[vulkan] Error: VkResult = %d\n", err);
	if (err < 0)
		abort();
}

// Validation layer callback
static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
	VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT messageType,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	void* pUserData)
{
	switch (messageSeverity)
	{
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
		std::cout << std::endl << "--- [ Validation layer Warning! ] ---" << std::endl;
		std::cout << pCallbackData->pMessage << std::endl;
		break;
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
		std::cout << std::endl << "--- [ Validation layer Error! ] ---" << std::endl;
		std::cout << pCallbackData->pMessage << std::endl;
		throw std::runtime_error("Validation layer reported an error!");
		break;
	}

	return VK_TRUE;
}

namespace NVulkanEngine
{
	void CVulkanGraphicsEngine::Initialize()
	{
		InitWindow();
		InitVulkan();
		CreateGraphicsContext();
		InitSwapchain();
		InitManagers();
		InitCamera();
		InitImGui();
	}

	void CVulkanGraphicsEngine::InitVulkan()
	{
		CreateVulkanInstance();
		SetupDebugMessenger();
		CreateVulkanSurface();
		SelectPhysicalDevice();
		CreateLogicalDevice();
		CreateCommandPool();
		CreateCommandBuffers();
		CreateSyncObjects();
		CreateSamplers();
	}

	void CVulkanGraphicsEngine::CreateScene()
	{
		CreateModels();
		CreateAttachments();
		InitDrawPasses();

		m_IsRunning = true;
	}

	void CVulkanGraphicsEngine::Cleanup()
	{
		vkDeviceWaitIdle(m_VulkanDevice);

		CleanupDrawPasses();
		CleanupManagers();
		CleanupImGui();
		CleanupVulkan();
		CleanupWindow();
	}

	void CVulkanGraphicsEngine::ResizeFrame()
	{
		vkDeviceWaitIdle(m_VulkanDevice);

		// These don't directly depend on resolution but we need to recreate them anyway
		m_Swapchain->Recreate(m_Context);
		CleanupSyncObjects();
		CreateSyncObjects();

		// These do depend on resolution
		m_AttachmentManager->Cleanup(m_Context);
		CleanupDrawPasses();

		g_DisplayWidth  = m_NewRenderResolution.width;
		g_DisplayHeight = m_NewRenderResolution.height;
		m_Context->SetRenderResolution(VkExtent2D(g_DisplayWidth, g_DisplayHeight));

		// Order of init important here
		CreateAttachments();
		InitDrawPasses();

		m_NeedsResize = false;
	}

	void CVulkanGraphicsEngine::ResizeGLFWFrame(GLFWwindow* window, int newWidth, int newHeight)
	{
		m_NeedsResize = true;
		m_NewRenderResolution = VkExtent2D(newWidth, newHeight);
	}

	void CVulkanGraphicsEngine::ProcessGLFWKeyboardInputs(GLFWwindow* window, int key, int scancode, int action, int mods)
	{
		m_InputManager->ProcessKeyboardInputs(window, key, scancode, action, mods);
	}

	void CVulkanGraphicsEngine::ProcessGLFWMouseInput(GLFWwindow* window, int button, int action, int mods)
	{
		m_InputManager->ProcessMouseInput(window, button, action, mods);
	}

	void CVulkanGraphicsEngine::InitWindow()
	{
		glfwInit();

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

		m_Window = glfwCreateWindow(g_DisplayWidth, g_DisplayHeight, "Vulkan", nullptr, nullptr);

		if (m_Window == NULL)
		{
			throw std::runtime_error("Failed to init Window");
		}

		GLFWkeyfun keyboardCallback = ([](GLFWwindow* window, int key, int scancode, int action, int mods)
		{
			CVulkanGraphicsEngine& graphicsEngine = *static_cast<CVulkanGraphicsEngine*>(glfwGetWindowUserPointer(window));
			graphicsEngine.ProcessGLFWKeyboardInputs(window, key, scancode, action, mods);
		});

		GLFWmousebuttonfun mouseCallback = ([](GLFWwindow* window, int button, int action, int mods)
		{
			CVulkanGraphicsEngine& graphicsEngine = *static_cast<CVulkanGraphicsEngine*>(glfwGetWindowUserPointer(window));
			graphicsEngine.ProcessGLFWMouseInput(window, button, action, mods);
		});

		glfwSetKeyCallback(m_Window, keyboardCallback);
		glfwSetMouseButtonCallback(m_Window, mouseCallback);

		GLFWframebuffersizefun windowScaleCallback = ([](GLFWwindow* window, int newWidth, int newHeight)
		{
			CVulkanGraphicsEngine& graphicsEngine = *static_cast<CVulkanGraphicsEngine*>(glfwGetWindowUserPointer(window));
			graphicsEngine.ResizeGLFWFrame(window, newWidth, newHeight);
		});


		glfwSetFramebufferSizeCallback(m_Window, windowScaleCallback);
	}

	void CVulkanGraphicsEngine::InitSwapchain()
	{
		m_Swapchain = new CSwapchain();
		m_Swapchain->Create(m_Context);
	}

	void CVulkanGraphicsEngine::InitManagers()
	{
		m_InputManager = new CInputManager();
		m_ModelManager = new CModelManager();
		m_AttachmentManager = new CAttachmentManager();

		// For mouse and keyboard callbacks
		glfwSetWindowUserPointer(m_Window, this);
	}

	void CVulkanGraphicsEngine::InitCamera()
	{
		CCamera* camera = m_InputManager->GetCamera();
		
		glm::vec3 cameraOriginPosition = glm::vec3(-300.0f, 250, -7.0f); // Arbitrary camera start position
		glm::vec3 cameraViewDirection  = glm::vec3(1.0f, 0.0f, 0.0f);
		glm::vec3 cameraUpDirection    = glm::vec3(0.0f, 1.0f, 0.0f);
		
		camera->SetPosition(cameraOriginPosition);
		camera->SetDirection(cameraViewDirection);
		camera->SetUp(cameraUpDirection);

		camera->UpdateCamera(m_Context);
	}

	void CVulkanGraphicsEngine::CleanupManagers()
	{
		m_ModelManager->Cleanup(m_Context);
		m_AttachmentManager->Cleanup(m_Context);

		delete m_InputManager;
		delete m_ModelManager;
		delete m_AttachmentManager;
	};


	void CVulkanGraphicsEngine::CleanupDrawPasses()
	{
		for (uint32_t i = 0; i < m_DrawPasses.size(); i++)
		{
			m_DrawPasses[i]->CleanupPass(m_Context);
		}
	}

	void CVulkanGraphicsEngine::CleanupSyncObjects()
	{
		for (size_t i = 0; i < g_MaxFramesInFlight; i++)
		{
			vkDestroySemaphore(m_VulkanDevice, m_ImageAvailableSemaphores[i], nullptr);
			vkDestroySemaphore(m_VulkanDevice, m_RenderFinishedSemaphores[i], nullptr);
			vkDestroyFence(m_VulkanDevice, m_InFlightFences[i], nullptr);
		}
	}

	void CVulkanGraphicsEngine::CleanupVulkan()
	{
		vkDestroyDescriptorPool(m_VulkanDevice, m_ImGuiDescriptorPool, nullptr);

		vkDestroySampler(m_VulkanDevice, m_LinearClamp, nullptr);
		vkDestroySampler(m_VulkanDevice, m_LinearRepeat, nullptr);

		CleanupSyncObjects();

		m_Swapchain->Cleanup(m_Context);
		delete m_Swapchain;

		vkDestroyCommandPool(m_VulkanDevice, m_CommandPool, nullptr);

		vkDestroyDevice(m_VulkanDevice, nullptr);

		if (g_EnableValidationLayers)
		{
			DestroyDebugUtilsMessengerEXT(m_DebugMessenger, nullptr);
		}

		vkDestroySurfaceKHR(m_VulkanInstance, m_VulkanSurface, nullptr);
		vkDestroyInstance(m_VulkanInstance, nullptr);
	}

	void CVulkanGraphicsEngine::CleanupWindow()
	{
		glfwDestroyWindow(m_Window);
	}

	void CVulkanGraphicsEngine::CleanupImGui()
	{
		ImGui_ImplVulkan_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();
	}

	bool CVulkanGraphicsEngine::CheckDeviceExtensionSupport(VkPhysicalDevice device, const std::vector<const char*> deviceExtensions)
	{
		std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

		uint32_t extensionCount;
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
		std::vector<VkExtensionProperties> availableExtensions(extensionCount);
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

		for (const auto& extension : availableExtensions)
		{
			requiredExtensions.erase(extension.extensionName);
		}

		return requiredExtensions.empty();
	}

	bool CVulkanGraphicsEngine::IsDeviceSuitable(VkPhysicalDevice device, VkSurfaceKHR surface, const std::vector<const char*> deviceExtensions)
	{
		SVulkanQueueFamilyIndices indices = FindVulkanQueueFamilies(device, surface);

		bool extensionsSupported = CheckDeviceExtensionSupport(device, deviceExtensions);

		VkPhysicalDeviceFeatures supportedFeatures;
		vkGetPhysicalDeviceFeatures(device, &supportedFeatures);

		return indices.IsComplete() && extensionsSupported && supportedFeatures.samplerAnisotropy;
	};

	void CVulkanGraphicsEngine::PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo)
	{
		createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;

		// Just need warnings and errors
		createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		createInfo.flags = 0;

		// Our custom callback
		createInfo.pfnUserCallback = debugCallback;
		createInfo.pUserData = nullptr; // Optional
	}

	VkResult CVulkanGraphicsEngine::CreateDebugUtilsMessengerEXT(const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger)
	{
		auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(m_VulkanInstance, "vkCreateDebugUtilsMessengerEXT");
		if (func != nullptr)
		{
			return func(m_VulkanInstance, pCreateInfo, pAllocator, pDebugMessenger);
		}
		else
		{
			return VK_ERROR_EXTENSION_NOT_PRESENT;
		}
	}

	VkResult CVulkanGraphicsEngine::DestroyDebugUtilsMessengerEXT(VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator)
	{
		auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(m_VulkanInstance, "vkDestroyDebugUtilsMessengerEXT");
		if (func != nullptr)
		{
			func(m_VulkanInstance, debugMessenger, pAllocator);
			return VK_SUCCESS;
		}
		return VK_SUCCESS;
	}

	void CVulkanGraphicsEngine::SetupDebugMessenger()
	{
		if (!g_EnableValidationLayers)
			return;

		VkDebugUtilsMessengerCreateInfoEXT createInfo{};
		PopulateDebugMessengerCreateInfo(createInfo);

		if (CreateDebugUtilsMessengerEXT(&createInfo, nullptr, &m_DebugMessenger) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to set up debug messenger!");
		}
	}

	void CVulkanGraphicsEngine::CreateVulkanInstance()
	{

		VkApplicationInfo appInfo{};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = "Hello Triangle";
		appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.pEngineName = "No Engine";
		appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.apiVersion = VK_API_VERSION_1_3;

		VkInstanceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.pApplicationInfo = &appInfo;

		if (g_EnableValidationLayers && !CheckValidationLayerSupport())
		{
			throw std::runtime_error("Validation layers requested, but not available!");
		}

		VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
		if (g_EnableValidationLayers)
		{
			createInfo.enabledLayerCount = static_cast<uint32_t>(g_ValidationLayers.size());
			createInfo.ppEnabledLayerNames = g_ValidationLayers.data();

			PopulateDebugMessengerCreateInfo(debugCreateInfo);
			createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
		}
		else
		{
			createInfo.enabledLayerCount = 0;
		}

		std::vector<const char*> instanceExtensions = GetRequiredInstanceExtensions();

#if defined(_DEBUG)
		std::cout << "Required instance extensions:\n";
		for (const auto& SDLextension : instanceExtensions)
		{
			std::cout << '\t' << SDLextension << '\n';
		}
#endif // _DEBUG

		uint32_t extensionCount = 0;
		vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
		std::vector<VkExtensionProperties> extensions(extensionCount);
		vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());

#if defined(_DEBUG)
		std::cout << "Available instance extensions:\n";
		for (const auto& extension : extensions)
		{
			std::cout << '\t' << extension.extensionName << '\n';
		}
#endif // _DEBUG

	    // Check required extensions for GLFW are present
		for (uint32_t i = 0; i < instanceExtensions.size(); i++)
		{
			bool found = false;
			for (const auto& extension : extensions)
			{
				if (strcmp(instanceExtensions[i], extension.extensionName))
				{
					found = true;
				}
			}
			if (!found)
			{
				throw std::runtime_error("missing vulkan extension");
			}
		}

#if defined(_DEBUG)
		std::cout << "\nInstance extension requirements fulfilled!\n" << std::endl;
#endif // _DEBUG

		createInfo.enabledExtensionCount = static_cast<uint32_t>(instanceExtensions.size());
		createInfo.ppEnabledExtensionNames = instanceExtensions.data();
		VkResult result = vkCreateInstance(&createInfo, nullptr, &m_VulkanInstance);
		if (result != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create instance!");
		}
	}

	void CVulkanGraphicsEngine::SelectPhysicalDevice()
	{
		uint32_t deviceCount = 0;
		vkEnumeratePhysicalDevices(m_VulkanInstance, &deviceCount, nullptr);

		if (deviceCount == 0)
		{
			throw std::runtime_error("failed to find GPUs with Vulkan support!");
		}

		std::vector<VkPhysicalDevice> devices(deviceCount);
		vkEnumeratePhysicalDevices(m_VulkanInstance, &deviceCount, devices.data());

		VkPhysicalDevice physicalDeviceToBeSelected = VK_NULL_HANDLE;

#if defined(_DEBUG)
		std::cout << "Required device extensions:\n";
		for (const auto& deviceExtension : g_DeviceExtensions)
		{
			std::cout << '\t' << deviceExtension << '\n';
		}
#endif // _DEBUG

#if defined(_DEBUG)
		std::cout << "\nAvilable devices : " << std::endl;
#endif 
		for (size_t i = 0; i < devices.size(); i++)
		{
			const VkPhysicalDevice& device = devices[i];

			VkPhysicalDeviceProperties deviceProperties;
			VkPhysicalDeviceFeatures deviceFeatures;
			vkGetPhysicalDeviceProperties(device, &deviceProperties);
			vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

			uint32_t queueFamilyCount = 0;
			vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

			std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
			vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

			std::optional<uint32_t> physicalDeviceId;

#if defined(_DEBUG)
			std::cout << "\t" << i << " [" << deviceProperties.vendorID << " - " << deviceProperties.deviceName << "]" << std::endl;
#endif

			// Pick first device that has support for graphics queue
			if (IsDeviceSuitable(device, m_VulkanSurface, g_DeviceExtensions))
			{
#if defined(_DEBUG)
				std::cout << "\nAll extensions supported! Selecting physical device with ID: " << i << "..." << std::endl;
#endif
				m_PhysicalDevice = device;
				break;
			}

		}

		if (m_PhysicalDevice == VK_NULL_HANDLE)
		{
			throw std::runtime_error("failed to select a physical device!");
		}

	}

	void CVulkanGraphicsEngine::CreateLogicalDevice()
	{
		m_QueueFamilies = FindVulkanQueueFamilies(m_PhysicalDevice, m_VulkanSurface);

		std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
		std::set<uint32_t> uniqueQueueFamilies = { m_QueueFamilies.m_GraphicsFamily.value(), m_QueueFamilies.m_PresentFamily.value() };

		float queuePriority = 1.0f;
		for (uint32_t queueFamily : uniqueQueueFamilies)
		{
			VkDeviceQueueCreateInfo queueCreateInfo{};
			queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queueCreateInfo.queueFamilyIndex = queueFamily;
			queueCreateInfo.queueCount = 1;
			queueCreateInfo.pQueuePriorities = &queuePriority;

			queueCreateInfos.push_back(queueCreateInfo);
		}

		VkPhysicalDeviceRobustness2FeaturesEXT vulkanRobustnessFeatures{};
		vulkanRobustnessFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ROBUSTNESS_2_FEATURES_EXT;
		vulkanRobustnessFeatures.nullDescriptor = VK_TRUE;

		VkPhysicalDeviceVulkan13Features vulkan13Features{};
		vulkan13Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
		vulkan13Features.dynamicRendering = VK_TRUE;
		vulkan13Features.robustImageAccess = VK_TRUE;
		vulkan13Features.pNext = &vulkanRobustnessFeatures;

		VkPhysicalDeviceFeatures deviceFeatures{};
		deviceFeatures.robustBufferAccess = VK_TRUE;
		//deviceFeatures.samplerAnisotropy = VK_TRUE;

		VkDeviceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());;
		createInfo.pQueueCreateInfos = queueCreateInfos.data();
		createInfo.pEnabledFeatures = &deviceFeatures;
		createInfo.pNext = &vulkan13Features;

		createInfo.enabledExtensionCount = (uint32_t)g_DeviceExtensions.size();
		createInfo.ppEnabledExtensionNames = g_DeviceExtensions.data();

		if (g_EnableValidationLayers) {
			createInfo.enabledLayerCount = static_cast<uint32_t>(g_ValidationLayers.size());
			createInfo.ppEnabledLayerNames = g_ValidationLayers.data();
		}
		else {
			createInfo.enabledLayerCount = 0;
		}

		if (vkCreateDevice(m_PhysicalDevice, &createInfo, nullptr, &m_VulkanDevice) != VK_SUCCESS) {
			throw std::runtime_error("failed to create logical device!");
		}

		vkGetDeviceQueue(m_VulkanDevice, m_QueueFamilies.m_GraphicsFamily.value(), 0, &m_GraphicsQueue);
		vkGetDeviceQueue(m_VulkanDevice, m_QueueFamilies.m_PresentFamily.value(), 0, &m_PresentQueue);

#if defined(_DEBUG)
		std::cout << "Succesfully created logical device and queues!\n" << std::endl;
#endif
	}

	void CVulkanGraphicsEngine::CreateVulkanSurface()
	{
		glfwCreateWindowSurface(m_VulkanInstance, m_Window, nullptr, &m_VulkanSurface);
	}

	void CVulkanGraphicsEngine::CreateCommandPool()
	{
		SVulkanQueueFamilyIndices queueFamilyIndices = FindVulkanQueueFamilies(m_PhysicalDevice, m_VulkanSurface);

		VkCommandPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		poolInfo.queueFamilyIndex = queueFamilyIndices.m_GraphicsFamily.value();

		if (vkCreateCommandPool(m_VulkanDevice, &poolInfo, nullptr, &m_CommandPool) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create command pool!");
		}
	}

	void CVulkanGraphicsEngine::CreateCommandBuffers()
	{
		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandPool = m_CommandPool;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

		m_CommandBuffers.resize(g_MaxFramesInFlight);
		allocInfo.commandBufferCount = (uint32_t)m_CommandBuffers.size();

		if (vkAllocateCommandBuffers(m_VulkanDevice, &allocInfo, m_CommandBuffers.data()) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to allocate command buffers!");
		}

#if defined(_DEBUG)
		std::cout << "Command Pool & Buffers created!" << std::endl;
#endif
	}

	void CVulkanGraphicsEngine::CreateSyncObjects()
	{
		m_ImageAvailableSemaphores.resize(g_MaxFramesInFlight);
		m_RenderFinishedSemaphores.resize(g_MaxFramesInFlight);
		m_InFlightFences.resize(g_MaxFramesInFlight);

		VkSemaphoreCreateInfo semaphoreInfo{};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		VkFenceCreateInfo fenceInfo{};
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		for (size_t i = 0; i < g_MaxFramesInFlight; i++)
		{
			if (vkCreateSemaphore(m_VulkanDevice, &semaphoreInfo, nullptr, &m_ImageAvailableSemaphores[i]) != VK_SUCCESS ||
				vkCreateSemaphore(m_VulkanDevice, &semaphoreInfo, nullptr, &m_RenderFinishedSemaphores[i]) != VK_SUCCESS ||
				vkCreateFence(m_VulkanDevice, &fenceInfo, nullptr, &m_InFlightFences[i]) != VK_SUCCESS)
			{
				throw std::runtime_error("failed to create semaphores!");
			}
		}
	}

	void CVulkanGraphicsEngine::CreateSamplers()
	{
		VkSamplerCreateInfo samplerInfo{};
		samplerInfo.sType                   = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerInfo.minFilter               = VK_FILTER_LINEAR;
		samplerInfo.magFilter               = VK_FILTER_LINEAR;
		samplerInfo.mipmapMode              = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		samplerInfo.addressModeU            = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
		samplerInfo.addressModeV            = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
		samplerInfo.addressModeW            = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
		samplerInfo.mipLodBias              = 0.0f;
		samplerInfo.maxAnisotropy           = 1.0f;
		samplerInfo.minLod                  = 0.0f;
		samplerInfo.maxLod                  = 1.0f;
		samplerInfo.borderColor             = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
		samplerInfo.unnormalizedCoordinates = VK_FALSE;
		if (vkCreateSampler(m_VulkanDevice, &samplerInfo, nullptr, &m_LinearClamp) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create sampler for geometry render pass!");
		}

		samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		if (vkCreateSampler(m_VulkanDevice, &samplerInfo, nullptr, &m_LinearRepeat) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create sampler for geometry render pass!");
		}

	}

	void CVulkanGraphicsEngine::CreateAttachments()
	{
		m_AttachmentManager->AddAttachment(
			m_Context,
			"GBuffer - Positions",
			EAttachmentIndices::Positions,
			m_LinearClamp,
			VK_FORMAT_R16G16B16A16_SFLOAT,
			VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			m_Context->GetRenderResolution().width,
			m_Context->GetRenderResolution().height);

		m_AttachmentManager->AddAttachment(
			m_Context,
			"GBuffer - Normals",
			EAttachmentIndices::Normals,
			m_LinearClamp,
			VK_FORMAT_R16G16B16A16_SFLOAT,
			VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			m_Context->GetRenderResolution().width,
			m_Context->GetRenderResolution().height);

		m_AttachmentManager->AddAttachment(
			m_Context,
			"GBuffer - Albedo",
			EAttachmentIndices::Albedo, 
			m_LinearClamp,
			VK_FORMAT_R8G8B8A8_UNORM,
			VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			m_Context->GetRenderResolution().width,
			m_Context->GetRenderResolution().height);

		m_AttachmentManager->AddAttachment(
			m_Context,
			"GBuffer - Depth",
			EAttachmentIndices::Depth,
			m_LinearClamp,
			FindDepthFormat(m_Context->GetPhysicalDevice()),
			VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
			VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
			m_Context->GetRenderResolution().width,
			m_Context->GetRenderResolution().height);

		m_AttachmentManager->AddAttachment(
			m_Context,
			"Shadow Map",
			EAttachmentIndices::ShadowMap,
			m_LinearClamp,
			VK_FORMAT_D32_SFLOAT,
			VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
			VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
			(uint32_t)1024,
			(uint32_t)1024);

		m_AttachmentManager->AddAttachment(
			m_Context,
			"Atmospherics SkyBox",
			EAttachmentIndices::AtmosphericsSkyBox,
			m_LinearClamp,
			VK_FORMAT_R8G8B8A8_UNORM,
			VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			m_Context->GetRenderResolution().width,
			m_Context->GetRenderResolution().height);

		m_AttachmentManager->AddAttachment(
			m_Context,
			"Scene Color",
			EAttachmentIndices::SceneColor,
			m_LinearClamp,
			VK_FORMAT_R8G8B8A8_UNORM,
			VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			m_Context->GetRenderResolution().width,
			m_Context->GetRenderResolution().height);
	}

	void CVulkanGraphicsEngine::InitImGui()
	{
		// Setup Dear ImGui context
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO();
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // IF using Docking Branch

		ImGui_ImplGlfw_InitForVulkan(m_Window, true);

		std::array<VkDescriptorPoolSize, 2> poolSizes{};
		poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		poolSizes[0].descriptorCount = (uint32_t)EAttachmentIndices::Count * 2;
		poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		poolSizes[1].descriptorCount = (uint32_t)EAttachmentIndices::Count * m_Swapchain->GetMinImageCount() + 1;

		VkDescriptorPoolCreateInfo poolInfo{};
		poolInfo.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.flags         = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
		poolInfo.poolSizeCount = (uint32_t)poolSizes.size();
		poolInfo.pPoolSizes    = poolSizes.data();
		poolInfo.maxSets       = (uint32_t)EAttachmentIndices::Count * 2;

		if (vkCreateDescriptorPool(m_Context->GetLogicalDevice(), &poolInfo, nullptr, &m_ImGuiDescriptorPool) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create descriptor pool!");
		}

		VkFormat swapchain_format = m_Swapchain->GetSwapchainFormat();

		VkPipelineRenderingCreateInfoKHR pipeline_rendering_create_info{};
		pipeline_rendering_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR;
		pipeline_rendering_create_info.colorAttachmentCount = 1;
		pipeline_rendering_create_info.pColorAttachmentFormats = &swapchain_format;

		ImGui_ImplVulkan_InitInfo init_info{};
		init_info.Instance                    = m_VulkanInstance;
		init_info.PhysicalDevice              = m_PhysicalDevice;
		init_info.Device                      = m_VulkanDevice;
		init_info.QueueFamily                 = m_QueueFamilies.m_GraphicsFamily.value();
		init_info.Queue                       = m_GraphicsQueue;
		init_info.PipelineCache               = VK_NULL_HANDLE;
		init_info.DescriptorPool              = m_ImGuiDescriptorPool;
		init_info.PipelineRenderingCreateInfo = pipeline_rendering_create_info;
		init_info.UseDynamicRendering         = true;
		init_info.RenderPass                  = nullptr; // Not needed since using dynamic rendering
		init_info.Subpass                     = 0;
		init_info.MinImageCount               = m_Swapchain->GetMinImageCount();
		init_info.ImageCount                  = m_Swapchain->GetMinImageCount() + 1;
		init_info.MSAASamples                 = VK_SAMPLE_COUNT_1_BIT;
		init_info.Allocator                   = nullptr; // For now. Add to enable validation layer debugging?
		init_info.CheckVkResultFn             = check_vk_result; // For now
		ImGui_ImplVulkan_Init(&init_info);
	}

	void CVulkanGraphicsEngine::CreateGraphicsContext()
	{
		m_Context = new CGraphicsContext();
		m_Context->CreateContext(
			m_VulkanInstance,
			m_VulkanSurface,
			m_Window,
			m_PhysicalDevice,
			m_VulkanDevice,
			m_CommandPool,
			m_GraphicsQueue,
			m_PresentQueue,
			m_LinearClamp,
			m_LinearRepeat,
			VkExtent2D(g_DisplayWidth, g_DisplayHeight));
	}

	void CVulkanGraphicsEngine::CreateModels()
	{
		m_ModelManager->AllocateModelDescriptorPool(m_Context);

		for (uint32_t i = 0; i < m_ModelManager->GetNumModels(); i++)
		{
			CModel* model = m_ModelManager->GetModel(i);

			if (model->UsesModelTexture())
			{
				CTexture* modelTexture = new CTexture();
				modelTexture->SetGenerateMipmaps(false);
				modelTexture->CreateTexture(m_Context, model->GetModelTexturePath(), VK_FORMAT_R8G8B8A8_SRGB);

				model->SetModelTexture(modelTexture);
			}

			model->CreateModelMeshes(m_Context);
		}
	}

	void CVulkanGraphicsEngine::InitDrawPasses()
	{
		m_DrawPasses[(uint32_t)EDrawPasses::Geometry]     = new CGeometryPass();
		m_DrawPasses[(uint32_t)EDrawPasses::Shadows]      = new CShadowPass();
		m_DrawPasses[(uint32_t)EDrawPasses::Atmospherics] = new CAtmosphericsPass();
		m_DrawPasses[(uint32_t)EDrawPasses::Lighting]     = new CLightingPass();

		SGraphicsManagers managers{};
		managers.m_InputManager      = m_InputManager;
		managers.m_Modelmanager      = m_ModelManager;
		managers.m_AttachmentManager = m_AttachmentManager;

		for (uint32_t i = 0; i < m_DrawPasses.size(); i++)
		{
			CDrawPass* drawPass = m_DrawPasses[i];
			if (drawPass)
				drawPass->InitPass(m_Context, &managers);
		}
	}

	void CVulkanGraphicsEngine::RecordDrawPasses(VkCommandBuffer commandBuffer)
	{
		SGraphicsContext context{};
		context.m_VulkanDevice        = m_Context->GetLogicalDevice();
		context.m_RenderResolution    = m_Context->GetRenderResolution();
		context.m_DeltaTime           = m_Context->GetDeltaTime();
		context.m_FrameIndex          = m_Context->GetFrameIndex();
		context.m_SwapchainImageIndex = m_Context->GetSwapchainImageIndex();

		SGraphicsManagers managers{};
		managers.m_InputManager      = m_InputManager;
		managers.m_Modelmanager      = m_ModelManager;
		managers.m_AttachmentManager = m_AttachmentManager;

		for (uint32_t i = 0; i < m_DrawPasses.size(); i++)
		{
			CDrawPass* drawPass = m_DrawPasses[i];
			if (drawPass)
				drawPass->DrawPass(m_Context, &managers, commandBuffer);
		}
	}

	void CVulkanGraphicsEngine::DoImGuiViewport()
	{
		ImGui::DockSpaceOverViewport(ImGui::GetMainViewport());
		ImGui::Begin("Viewport");
		VkDescriptorSet sceneColorDescriptor = m_AttachmentManager->TransitionAttachment(m_CommandBuffers[m_FrameIndex], EAttachmentIndices::SceneColor, VK_ATTACHMENT_LOAD_OP_LOAD, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL).m_ImguiDescriptor;
		ImGui::Image((ImTextureID)sceneColorDescriptor, ImGui::GetContentRegionAvail());
		ImGui::End();
		ImGui::Begin("Textures");

		static bool selected[(uint32_t)EAttachmentIndices::Count] = {};
		static int selectedId = -1;

		if (ImGui::CollapsingHeader("List", ImGuiTreeNodeFlags_DefaultOpen))
		{
			static char inputText[64] = "";
			ImGui::InputText("Filter", inputText, 64);
			for (uint32_t i = 0; i < m_AttachmentManager->GetAttachments().size(); i++)
			{
				SRenderAttachment attachment = m_AttachmentManager->GetAttachment((EAttachmentIndices)i);

				ImGui::Selectable(attachment.m_DebugName, &selected[i]);
				if (selected[i])
					selectedId = i;
			}
		}
		if (ImGui::CollapsingHeader("Selected", ImGuiTreeNodeFlags_DefaultOpen))
		{
			if (selectedId == -1)
				ImGui::Text("No Texture selected. Select one from the list above to inspect!");
			else
			{
				SRenderAttachment attachment = m_AttachmentManager->GetAttachment((EAttachmentIndices)selectedId);
				ImGui::Text("Name: %s", attachment.m_DebugName);
				static float brightness = 1.0f;
				ImGui::SliderFloat("Brightness", &brightness, 0.0f, 1.0f);
				ImGui::Image(attachment.m_ImguiDescriptor, ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y / 2.0f), ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f), ImVec4(1.0f * brightness, 1.0f * brightness, 1.0f * brightness, 1.0f) );
				if (ImGui::Button("Remove Selection"))
				{
					selected[selectedId] = false;
					selectedId = -1;
				}

			}
		}

		ImGui::End();
	}

	void CVulkanGraphicsEngine::RenderImGuiDrawData(uint32_t imageIndex)
	{
		CSwapchain* swapchain = m_Swapchain;

		ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

		ImGui::Render();
		ImDrawData* main_draw_data = ImGui::GetDrawData();
		const bool main_is_minimized = (main_draw_data->DisplaySize.x <= 0.0f || main_draw_data->DisplaySize.y <= 0.0f);

		VkRenderingAttachmentInfo render_attachment_info{};

		VkRenderingAttachmentInfo swapChainInfo{};
		swapChainInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
		swapChainInfo.imageView = swapchain->GetSwapchainImageView(imageIndex);
		swapChainInfo.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		swapChainInfo.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
		swapChainInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

		std::vector<VkRenderingAttachmentInfo> colorAttachmentInfos = { swapChainInfo };

		VkRenderingInfo renderInfo{};
		renderInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
		renderInfo.renderArea.extent = m_Context->GetRenderResolution();
		renderInfo.renderArea.offset = { 0, 0 };
		renderInfo.layerCount = 1;
		renderInfo.colorAttachmentCount = static_cast<uint32_t>(colorAttachmentInfos.size());
		renderInfo.pColorAttachments = colorAttachmentInfos.data();
		renderInfo.pDepthAttachment = nullptr;

		SRenderAttachment swapchainAttachment{};
		swapchainAttachment.m_Format = swapchain->GetSwapchainFormat();
		swapchainAttachment.m_Image = swapchain->GetSwapchainImage(imageIndex);
		swapchainAttachment.m_ImageView = swapchain->GetSwapchainImageView(imageIndex);
		swapchainAttachment.m_ImageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		swapchainAttachment.m_RenderAttachmentInfo = swapChainInfo;
		swapchainAttachment.m_Memory = VK_NULL_HANDLE; // Not needed

		VkImage swapchainImage = swapchain->GetSwapchainImage(imageIndex);
		VkFormat swapchainFormat = swapchain->GetSwapchainFormat();
		TransitionImageLayout(m_CommandBuffers[m_FrameIndex], swapchainAttachment, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 1);

		// Record dear imgui primitives into command buffer
		vkCmdBeginRendering(m_CommandBuffers[m_FrameIndex], &renderInfo);
		ImGui_ImplVulkan_RenderDrawData(main_draw_data, m_CommandBuffers[m_FrameIndex]);
		vkCmdEndRendering(m_CommandBuffers[m_FrameIndex]);

		TransitionImageLayout(m_CommandBuffers[m_FrameIndex], swapchainAttachment, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, 1);
	}

	void CVulkanGraphicsEngine::AddModel(const std::string& modelpath)
	{
		m_ModelManager->AddModel(modelpath);
	}

	void CVulkanGraphicsEngine::SetModelTexture(const std::string& texturePath)
	{
		m_ModelManager->AddTexturePath(m_ModelManager->GetCurrentModelIndex(), texturePath);
	}


	void CVulkanGraphicsEngine::SetModelPosition(float x, float y, float z)
	{
		glm::vec3 translation = glm::vec3(x, y, z);

		m_ModelManager->AddPosition(m_ModelManager->GetCurrentModelIndex(), translation);
	}

	void CVulkanGraphicsEngine::SetModelRotation(float x, float y, float z)
	{
		glm::vec3 rotation = glm::vec3(x, y, z);

		m_ModelManager->AddRotation(m_ModelManager->GetCurrentModelIndex(), rotation);
	}

	void CVulkanGraphicsEngine::SetModelScaling(float x, float y, float z)
	{
		glm::vec3 scaling = glm::vec3(x, y, z);

		m_ModelManager->AddScaling(m_ModelManager->GetCurrentModelIndex(), scaling);
	}

	bool CVulkanGraphicsEngine::IsRunning()
	{
		return m_IsRunning;
	}

	static auto s_LastTime = std::chrono::high_resolution_clock::now();

	void CVulkanGraphicsEngine::DrawFrame()
	{
		if (glfwWindowShouldClose(m_Window))
		{
			m_IsRunning = false;
			return;
		}
		glfwPollEvents();

		vkWaitForFences(m_VulkanDevice, 1, &m_InFlightFences[m_FrameIndex], VK_TRUE, UINT64_MAX);

		uint32_t imageIndex = 0;
		VkResult swapchainResult = m_Swapchain->AcquireSwapchainImageIndex(m_Context, m_ImageAvailableSemaphores[m_FrameIndex], imageIndex);

		if (m_NeedsResize)
		{
			ResizeFrame();
			return;
		}

		auto newTime = std::chrono::high_resolution_clock::now();
		float deltatIme = std::chrono::duration<float, std::chrono::seconds::period>(newTime - s_LastTime).count();
		s_LastTime = newTime;

		m_Context->SetDeltaTime(deltatIme);
		m_Context->SetSwapchainImageIndex(imageIndex);

		// Update camera matrix based on user input
		m_InputManager->UpdateCameraTransforms(m_Context, deltatIme);

		// Only reset the fence if we are clear to submit work
		vkResetFences(m_VulkanDevice, 1, &m_InFlightFences[m_FrameIndex]);

		vkResetCommandBuffer(m_CommandBuffers[m_FrameIndex], 0);

		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = 0; // Optional
		beginInfo.pInheritanceInfo = nullptr; // Optional

		if (vkBeginCommandBuffer(m_CommandBuffers[m_FrameIndex], &beginInfo) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to begin recording command buffer!");
		}

		SetViewportScissor(m_CommandBuffers[m_FrameIndex], m_Context->GetRenderResolution());

		// Start the Dear ImGui frame
		ImGui_ImplVulkan_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		RecordDrawPasses(m_CommandBuffers[m_FrameIndex]);

		DoImGuiViewport();
		RenderImGuiDrawData(imageIndex);

		if (vkEndCommandBuffer(m_CommandBuffers[m_FrameIndex]) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to record command buffer!");
		}

		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

		VkSemaphore waitSemaphores[] = { m_ImageAvailableSemaphores[m_FrameIndex] };
		VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = waitSemaphores;
		submitInfo.pWaitDstStageMask = waitStages;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &m_CommandBuffers[m_FrameIndex];

		VkSemaphore signalSemaphores[] = { m_RenderFinishedSemaphores[m_FrameIndex] };
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = signalSemaphores;

		VkResult result = vkQueueSubmit(m_GraphicsQueue, 1, &submitInfo, m_InFlightFences[m_FrameIndex]);

		if(result != VK_SUCCESS)
		{
			throw std::runtime_error("failed to submit commandbuffer on graphics queue!");
		}

		m_Swapchain->Present(m_Context, signalSemaphores, imageIndex);

		m_FrameIndex = (m_FrameIndex + 1) % g_MaxFramesInFlight;
		m_Context->SetFrameIndex(m_FrameIndex);

	}
};

