#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vulkan/vulkan_core.h>
#include <optional>
#include <cstdlib>
#include <vector>
#include <set>
#include <iostream>
#include <fstream>
#include <array>


#include <algorithm> // std::clamp

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/hash.hpp>

#undef main

struct Vertex
{
	glm::vec3 position;
	glm::vec3 color;
	glm::vec2 texCoord;

	// Vertex bindings
	static VkVertexInputBindingDescription GetBindingDescription()
	{
		VkVertexInputBindingDescription bindingDescription;

		bindingDescription.binding = 0;
		bindingDescription.stride = sizeof(Vertex);
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX; // Move to the next data entry after each vertex

		return bindingDescription;
	};

	// Vertex attributes
	static std::array<VkVertexInputAttributeDescription, 3> GetAttributeDescriptions()
	{
		std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions{};

		// inPosition
		attributeDescriptions[0].binding = 0;
		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT; //vec2
		attributeDescriptions[0].offset = offsetof(Vertex, position);

		// inColor
		attributeDescriptions[1].binding = 0;
		attributeDescriptions[1].location = 1;
		attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT; //vec3
		attributeDescriptions[1].offset = offsetof(Vertex, color);

		// texCoord
		attributeDescriptions[2].binding = 0;
		attributeDescriptions[2].location = 2;
		attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
		attributeDescriptions[2].offset = offsetof(Vertex, texCoord);

		return attributeDescriptions;
	};

	bool operator==(const Vertex& other) const 
	{
		return position == other.position && color == other.color && texCoord == other.texCoord;
	}
};

namespace std
{
	template<> struct hash<Vertex>
	{
		size_t operator()(Vertex const& vertex) const
		{
			return (
				(hash<glm::vec3>()(vertex.position) ^
					(hash<glm::vec3>()(vertex.color) << 1)) >> 1) ^
				(hash<glm::vec2>()(vertex.texCoord) << 1);
		}
	};
}


class TriangleApp
{
public:
	void Run();


private:
	void InitWindow();
	void InitVulkan();
	void DrawFrame();
	void MainLoop();
	void CleanupApp();

	// Vulkan init stuff
	void CreateVulkanInstance();
	void SetupDebugMessenger();
	void CreateVulkanSurface();
	void SelectPhysicalDevice();
	void CreateLogicalDevice();
	void CreateSwapChain();
	void CreateSwapChainImageViews();
	void CreateRenderPass();
	void CreateDescriptorSetLayout();
	void CreateGraphicsPipeline();
	void CreateFrameBuffers();
	void CreateCommandPool();
	void CreateDepthResources();
	void CreateTextureImage();
	void CreateTextureImageView();
	void CreateTextureSampler();
	void LoadModel();
	void CreateVertexBuffer();
	void CreateIndexBuffer();
	void CreateUniformBuffers();
	void CreateDescriptorPool();
	void CreateDescriptorSets();
	void CreateCommandBuffers();
	void CreateSyncObjects();

	void CleanupSwapchain();
	void RecreateSwapChain();

	VkCommandBuffer BeginSingleTimeCommands();
	void EndSingleTimeCommands(VkCommandBuffer commandBuffer);
	void TransitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);

	void RecordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);
	void UpdateUniformBuffer(uint32_t currentImage);
	void CreateImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory);
	VkImageView CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);
	void CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
	void CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
	void CopyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);

	bool CheckValidationLayerSupport();
	std::vector<const char*> GetRequiredInstanceExtensions();

	uint32_t				     m_FrameIndex = 0;

	// Device
	VkInstance			         m_VulkanInstance       = VK_NULL_HANDLE;
	VkPhysicalDevice	         m_PhysicalDevice       = VK_NULL_HANDLE;
	VkDevice			         m_VulkanDevice	        = VK_NULL_HANDLE;
	VkSurfaceKHR		         m_VulkanSurface        = VK_NULL_HANDLE;

	// Queues
	VkQueue				         m_GraphicsQueue        = VK_NULL_HANDLE;
	VkQueue				         m_PresentQueue	        = VK_NULL_HANDLE;

	// Swapchain
	VkSwapchainKHR               m_Swapchain            = VK_NULL_HANDLE;
	VkFormat                     m_SwapchainImageFormat = VK_FORMAT_UNDEFINED;
	VkExtent2D                   m_SwapchainExtent      = { 0, 0 };

	// Swap chain images
	std::vector<VkImage>         m_SwapchainImages      = { VK_NULL_HANDLE };
	std::vector<VkImageView>     m_SwapchainImageViews  = { VK_NULL_HANDLE };

	// Render pass
	VkRenderPass			     m_RenderPass           = VK_NULL_HANDLE;

	// Descriptor Set Layout
	VkDescriptorSetLayout        m_DescriptorSetLayout  = VK_NULL_HANDLE;

	// Descriptor Pool & Sets
	VkDescriptorPool             m_DescriptorPool       = VK_NULL_HANDLE;
	std::vector<VkDescriptorSet> m_DescriptorSets       = { VK_NULL_HANDLE };

	// Pipeline Layout
	VkPipelineLayout             m_PipelineLayout       = VK_NULL_HANDLE;

	// Pipeline
	VkPipeline				     m_GraphicsPipeline     = VK_NULL_HANDLE;

	// Vertex & Index buffers
	std::vector<Vertex>          m_Vertices             = {};
	std::vector<uint32_t>        m_Indices              = {};
	VkDeviceMemory               m_VertexBufferMemory   = VK_NULL_HANDLE;
	VkBuffer                     m_VertexBuffer         = VK_NULL_HANDLE;
	VkDeviceMemory               m_IndexBufferMemory    = VK_NULL_HANDLE;
	VkBuffer                     m_IndexBuffer          = VK_NULL_HANDLE;

	// Texture image & view
	VkImage                      m_TextureImage         = VK_NULL_HANDLE;
	VkDeviceMemory               m_TextureImageMemory   = VK_NULL_HANDLE;
	VkImageView	                 m_TextureImageView = VK_NULL_HANDLE;

	// Texture sampler
	VkSampler                    m_TextureSampler       = VK_NULL_HANDLE;

	// Depth resources
	VkImage                      m_DepthImage           = VK_NULL_HANDLE;
	VkDeviceMemory               m_DepthImageMemory     = VK_NULL_HANDLE;
	VkImageView                  m_DepthImageView       = VK_NULL_HANDLE;

	// Uniform buffers
	std::vector<VkBuffer>        m_UniformBuffers	    = { VK_NULL_HANDLE };
	std::vector<VkDeviceMemory>  m_UniformBufferMemory  = { VK_NULL_HANDLE };

	// Framebuffer
	std::vector<VkFramebuffer>   m_Framebuffers         = { VK_NULL_HANDLE };

	// CommandBuffer
	VkCommandPool                m_CommandPool          = VK_NULL_HANDLE;
	std::vector<VkCommandBuffer> m_CommandBuffers       = { VK_NULL_HANDLE };

	// Synchronization
	std::vector<VkSemaphore>     m_ImageAvailableSemaphores;
	std::vector<VkSemaphore>     m_RenderFinishedSemaphores;
	std::vector<VkFence>         m_InFlightFences;

	// Misc
	VkDebugUtilsMessengerEXT     m_DebugMessenger       = VK_NULL_HANDLE;
	GLFWwindow*				     m_Window               = nullptr;
	bool					     m_ShouldQuit           = false;

	struct VulkanQueueFamilyIndices
	{
		std::optional<uint32_t> m_GraphicsFamily;
		std::optional<uint32_t> m_PresentFamily;

		bool IsComplete()
		{
			return m_GraphicsFamily.has_value() && m_PresentFamily.has_value();
		};
	};

	struct SwapChainSupportDetails 
	{
		VkSurfaceCapabilitiesKHR capabilities      = {};
		std::vector<VkSurfaceFormatKHR> formats    = {};
		std::vector<VkPresentModeKHR> presentModes = {};
	};

	SwapChainSupportDetails QuerySwapChainSupport(VkPhysicalDevice device)
	{
		SwapChainSupportDetails details;

		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, m_VulkanSurface, &details.capabilities);

		uint32_t formatCount;
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_VulkanSurface, &formatCount, nullptr);
		if (formatCount != 0)
		{
			details.formats.resize(formatCount);
			vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_VulkanSurface, &formatCount, details.formats.data());
		}

		uint32_t presentModeCount;
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_VulkanSurface, &presentModeCount, nullptr);
		if (presentModeCount != 0)
		{
			details.presentModes.resize(presentModeCount);
			vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_VulkanSurface, &presentModeCount, details.presentModes.data());
		}

		return details;
	}

	VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) 
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

	VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes)
	{
		for (const auto& availablePresentMode : availablePresentModes) 
		{
			if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) 
			{
				return availablePresentMode;
			}
		}
		
		return VK_PRESENT_MODE_FIFO_KHR;
	}

	VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) 
	{
		if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) 
		{
			return capabilities.currentExtent;
		}
		else 
		{
			int width, height;
			glfwGetWindowSize(m_Window, &width, &height);

			VkExtent2D actualExtent = 
			{
				static_cast<uint32_t>(width),
				static_cast<uint32_t>(height)
			};

			actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
			actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

			return actualExtent;
		}
	}

	bool HasStencilComponent(VkFormat format) 
	{
		return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
	}

	VkFormat FindDepthFormat() 
	{
		return FindSupportedFormat(
			{ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
			VK_IMAGE_TILING_OPTIMAL,
			VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
	}

	VkFormat FindSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features)
	{
		for (VkFormat format : candidates) 
		{
			VkFormatProperties props;
			vkGetPhysicalDeviceFormatProperties(m_PhysicalDevice, format, &props);

			if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) 
			{
				return format;
			}
			else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) 
			{
				return format;
			}
		}

		throw std::runtime_error("failed to find supported format!");
	}

	VulkanQueueFamilyIndices FindVulkanQueueFamilies(VkPhysicalDevice device)
	{
		VulkanQueueFamilyIndices indices;

		uint32_t queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

		std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

		int i = 0;
		for (const auto& queueFamily : queueFamilies) 
		{
			VkBool32 presentSupport = false;
			vkGetPhysicalDeviceSurfaceSupportKHR(device, i, m_VulkanSurface, &presentSupport);

			if (presentSupport)
			{
				indices.m_PresentFamily = i;
			}
			if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) 
			{
				indices.m_GraphicsFamily = i;
			}

			if (indices.IsComplete())
			{
				break;
			}

			i++;
		}

		return indices;
	};

	bool CheckDeviceExtensionSupport(VkPhysicalDevice device, const std::vector<const char*> deviceExtensions)
	{
		std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

		uint32_t extensionCount;
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
		std::vector<VkExtensionProperties> availableExtensions(extensionCount);
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

#if defined(_DEBUG)
		//std::cout << "\tAvailable device extensions:\n\t \t[";
		//for (const auto& deviceExtension : availableExtensions) 
		//{
		//	std::cout << deviceExtension.extensionName << ", ";
		//}
		//std::cout << "]" << std::endl;
#endif // _DEBUG

		for (const auto& extension : availableExtensions)
		{
			requiredExtensions.erase(extension.extensionName);
		}

		return requiredExtensions.empty();
	}

	bool IsDeviceSuitable(VkPhysicalDevice device, const std::vector<const char*> deviceExtensions) {
		VulkanQueueFamilyIndices indices = FindVulkanQueueFamilies(device);

		bool extensionsSupported = CheckDeviceExtensionSupport(device, deviceExtensions);
		bool swapChainAdequate = false;
		if (extensionsSupported)
		{
			SwapChainSupportDetails swapChainSupport = QuerySwapChainSupport(device);
			swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty() ;
		}

		VkPhysicalDeviceFeatures supportedFeatures;
		vkGetPhysicalDeviceFeatures(device, &supportedFeatures);

		return indices.IsComplete() && extensionsSupported && swapChainAdequate && supportedFeatures.samplerAnisotropy;
	};

	static std::vector<char> ReadFile(const std::string &filename)
	{
		std::ifstream file(filename, std::ios::ate | std::ios::binary);

		if (!file.is_open())
		{
			throw std::runtime_error(std::string("Failed to open shader file: " + filename).c_str());
		}

		size_t fileSize = (size_t)file.tellg();
		std::vector<char> buffer(fileSize);

		file.seekg(0);
		file.read(buffer.data(), fileSize);
		file.close();

#if defined(_DEBUG)
		std::cout << "Shader module [name: " << filename << ", size: " << fileSize << " bytes, ";
#endif

		return buffer;
	}

	VkShaderModule CreateShaderModule(const std::string &filename)
	{
		auto shaderCode = ReadFile(filename);

		VkShaderModuleCreateInfo createInfo {};
		createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createInfo.codeSize = shaderCode.size();
		createInfo.pCode = reinterpret_cast<const uint32_t*>(shaderCode.data());

		VkShaderModule shaderModule;
		if (vkCreateShaderModule(m_VulkanDevice, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
		{
			throw std::runtime_error(std::string("Failed to created shader module for '" + filename + "'").c_str());
		}

#if defined(_DEBUG)
		std::cout << "pointer: 0x" << reinterpret_cast<void*>(shaderCode.data()) << "] " << "loaded" << std::endl;
#endif

		return shaderModule;
	}


	void PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo)
	{
		createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;

		// Just need warnings and errors
		createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		//createInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;

		// Our custom callback
		createInfo.pfnUserCallback = debugCallback;
		createInfo.pUserData = nullptr; // Optional
	}

	VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) 
	{
		auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
		if (func != nullptr) 
		{
			return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
		}
		else 
		{
			return VK_ERROR_EXTENSION_NOT_PRESENT;
		}
	}

	void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) 
	{
		auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
		if (func != nullptr) 
		{
			func(instance, debugMessenger, pAllocator);
		}
	}

	int FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) 
	{

		VkPhysicalDeviceMemoryProperties memoryProperties;
		vkGetPhysicalDeviceMemoryProperties(m_PhysicalDevice, &memoryProperties);

		for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++)
		{
			// Is memory type supported
			if (typeFilter & (1 << i) && (memoryProperties.memoryTypes[i].propertyFlags & properties) == properties)
			{
				return i;
			}
		}

		return -1;
	}

	// Validation layer callback
	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT messageType,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		void* pUserData) 
	{
		throw std::runtime_error(pCallbackData->pMessage);
	}

};

