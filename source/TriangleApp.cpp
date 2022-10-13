#include "TriangleApp.hpp"

#include <iostream>
#include <glm.hpp>
#include <vulkan/vulkan.h>
#include <vector>
#include <set>
#include <array>

static const uint32_t WIDTH                = 800;
static const uint32_t HEIGHT               = 600;
static const uint16_t MAX_FRAMES_IN_FLIGHT = 2;

const std::vector<const char*> validationLayers = {
	"VK_LAYER_KHRONOS_validation"
};

const std::vector<const char*> m_DeviceExtensions =
{
	VK_KHR_SWAPCHAIN_EXTENSION_NAME,
};

#if defined(_DEBUG)
const bool g_EnableValidationLayers = true;
#else
const bool g_EnableValidationLayers = false;
#endif

struct Vertex
{
	glm::vec2 position;
	glm::vec3 color;

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
	static std::array<VkVertexInputAttributeDescription, 2> GetAttributeDescriptions() {

		std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions{};

		attributeDescriptions[0].binding = 0;
		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT; //vec2
		attributeDescriptions[0].offset = offsetof(Vertex, position);

		attributeDescriptions[1].binding = 0;
		attributeDescriptions[1].location = 1;
		attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT; //vec3
		attributeDescriptions[1].offset = offsetof(Vertex, color);

		return attributeDescriptions;
	};
};

const std::vector<Vertex> vertices =
{
	{{0.0f, -0.5f}, {1.0f, 0.0f, 0.0f}},
	{{0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}},
	{{-0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}}
};

void TriangleApp::Run()
{
	InitWindow();
	InitVulkan();
	MainLoop();
	CleanupApp();
}

void TriangleApp::InitWindow()
{
	SDL_Init(SDL_INIT_EVERYTHING);
	m_Window = SDL_CreateWindow(
		"Software Rasterizer",
		SDL_WINDOWPOS_UNDEFINED,
		SDL_WINDOWPOS_UNDEFINED,
		WIDTH,
		HEIGHT,
		SDL_WINDOW_SHOWN | SDL_WINDOW_VULKAN
	);
	SDL_SetWindowResizable(m_Window, SDL_TRUE);

	if (m_Window == NULL)
	{
		throw std::runtime_error("Failed to init Window");
	}
}

std::vector<const char*> TriangleApp::GetRequiredInstanceExtensions()
{
	uint32_t requiredSDLExtensionCount;

	if (!SDL_Vulkan_GetInstanceExtensions(m_Window, &requiredSDLExtensionCount, nullptr))
	{
		throw std::runtime_error("failed to get SDL instance extension count!");
	}

	std::vector<const char*> requiredSDLExtensions(requiredSDLExtensionCount);
	if (!SDL_Vulkan_GetInstanceExtensions(m_Window, &requiredSDLExtensionCount, requiredSDLExtensions.data()))
	{
		throw std::runtime_error("failed to get SDL instance extensions!");
	}

	std::vector<const char*> requiredOtherExtensions;

	if (g_EnableValidationLayers) {
		requiredOtherExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	}

	std::vector<const char*> requiredExtensions;
	requiredExtensions.insert(requiredExtensions.end(), requiredOtherExtensions.begin(), requiredOtherExtensions.end());
	requiredExtensions.insert(requiredExtensions.end(), requiredSDLExtensions.begin(), requiredSDLExtensions.end());

	return requiredExtensions;
}

void TriangleApp::CreateVulkanInstance()
{

	VkApplicationInfo appInfo{};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "Hello Triangle";
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = "No Engine";
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_0;

	VkInstanceCreateInfo createInfo {};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;

	if (g_EnableValidationLayers && !CheckValidationLayerSupport()) 
	{
		throw std::runtime_error("Validation layers requested, but not available!");
	}

	VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo {};
	if (g_EnableValidationLayers) 
	{
		createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
		createInfo.ppEnabledLayerNames = validationLayers.data();

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
	//std::cout << "Available instance extensions:\n";
	//for (const auto& extension : extensions)
	//{
	//	std::cout << '\t' << extension.extensionName << '\n';
	//}
#endif // _DEBUG

	// Check required extensions for SDL are present
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

	createInfo.enabledExtensionCount   = (uint32_t)instanceExtensions.size();
	createInfo.ppEnabledExtensionNames = instanceExtensions.data();

	if (vkCreateInstance(&createInfo, nullptr, &m_VulkanInstance) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create instance!");
	}
}

void TriangleApp::SetupDebugMessenger()
{
	if (!g_EnableValidationLayers)
		return;

	VkDebugUtilsMessengerCreateInfoEXT createInfo {};
	PopulateDebugMessengerCreateInfo(createInfo);

	if (CreateDebugUtilsMessengerEXT(m_VulkanInstance, &createInfo, nullptr, &m_DebugMessenger) != VK_SUCCESS) 
	{
		throw std::runtime_error("failed to set up debug messenger!");
	}
}

bool TriangleApp::CheckValidationLayerSupport()
{
	uint32_t layerCount;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

	std::vector<VkLayerProperties> availableLayers(layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

	// Find validation layer support
	for (const char* layerName : validationLayers)
	{
		bool layerFound = false;
		for (const auto& layerProperties : availableLayers)
		{
			if (strcmp(layerName, layerProperties.layerName) == 0) 
			{
				std::cout << "Validation layers enabled: " << layerName << " Enabled\n" << std::endl;
				layerFound = true;
				break;
			}
		}

		if (!layerFound) 
		{
			return false;
		}
	}

	return true;
}

void TriangleApp::CreateVulkanSurface()
{
	if (SDL_Vulkan_CreateSurface(m_Window, m_VulkanInstance, &m_VulkanSurface) != SDL_TRUE)
	{
		throw std::runtime_error("Failed to create SDL vulkan surface");
	}
}

void TriangleApp::SelectPhysicalDevice()
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
	for (const auto& deviceExtension : m_DeviceExtensions)
	{
		std::cout << '\t' << deviceExtension << '\n';
	}
#endif // _DEBUG

#if defined(_DEBUG)
	std::cout << "\nAvilable devices : "<< std::endl;
#endif 
	for (size_t i = 0; i < devices.size(); i++) 
	{
		const VkPhysicalDevice &device = devices[i];

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
		if (IsDeviceSuitable(device, m_DeviceExtensions))
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

void TriangleApp::CreateLogicalDevice()
{	
	VulkanQueueFamilyIndices queueIndices = FindVulkanQueueFamilies(m_PhysicalDevice);
 
	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	std::set<uint32_t> uniqueQueueFamilies = { queueIndices.m_GraphicsFamily.value(), queueIndices.m_PresentFamily.value() };

	float queuePriority = 1.0f;
	for (uint32_t queueFamily : uniqueQueueFamilies)
	{
		VkDeviceQueueCreateInfo queueCreateInfo {};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = queueFamily;
		queueCreateInfo.queueCount = 1;
		queueCreateInfo.pQueuePriorities = &queuePriority;

		queueCreateInfos.push_back(queueCreateInfo);
	}

	VkPhysicalDeviceFeatures deviceFeatures {};

	VkDeviceCreateInfo createInfo {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());;
	createInfo.pQueueCreateInfos = queueCreateInfos.data();
	createInfo.pEnabledFeatures = &deviceFeatures;

	createInfo.enabledExtensionCount   = (uint32_t) m_DeviceExtensions.size();
	createInfo.ppEnabledExtensionNames = m_DeviceExtensions.data();

	if (g_EnableValidationLayers) {
		createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
		createInfo.ppEnabledLayerNames = validationLayers.data();
	}
	else {
		createInfo.enabledLayerCount = 0;
	}

	if (vkCreateDevice(m_PhysicalDevice, &createInfo, nullptr, &m_VulkanDevice) != VK_SUCCESS) {
		throw std::runtime_error("failed to create logical device!");
	}

	vkGetDeviceQueue(m_VulkanDevice, queueIndices.m_GraphicsFamily.value(), 0, &m_GraphicsQueue);
	vkGetDeviceQueue(m_VulkanDevice, queueIndices.m_PresentFamily.value(), 0, &m_PresentQueue);

#if defined(_DEBUG)
	std::cout << "Succesfully created logical device and queues!\n" << std::endl;
#endif
}

void TriangleApp::CreateSwapChain()
{
	SwapChainSupportDetails swapChainSupport = QuerySwapChainSupport(m_PhysicalDevice);

	VkSurfaceFormatKHR surfaceFormat = ChooseSwapSurfaceFormat(swapChainSupport.formats);
	VkPresentModeKHR presentMode = ChooseSwapPresentMode(swapChainSupport.presentModes);
	VkExtent2D extent = ChooseSwapExtent(swapChainSupport.capabilities);

	// Request one more image than the minium supported
	uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
	if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) 
	{
		imageCount = swapChainSupport.capabilities.maxImageCount;
	}

	VkSwapchainCreateInfoKHR createInfo {};
	createInfo.sType   = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface = m_VulkanSurface;

	createInfo.minImageCount    = imageCount;
	createInfo.imageFormat      = surfaceFormat.format;
	createInfo.imageColorSpace  = surfaceFormat.colorSpace;
	createInfo.imageExtent      = extent;
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	VulkanQueueFamilyIndices indices = FindVulkanQueueFamilies(m_PhysicalDevice);
	uint32_t queueFamilyIndices[] = { indices.m_GraphicsFamily.value(), indices.m_PresentFamily.value() };
	if (indices.m_GraphicsFamily != indices.m_PresentFamily) 
	{
		// Graphics & Presentation are in different queue families
		createInfo.imageSharingMode      = VK_SHARING_MODE_CONCURRENT;
		createInfo.queueFamilyIndexCount = sizeof(queueFamilyIndices) / sizeof(queueFamilyIndices[0]);
		createInfo.pQueueFamilyIndices   = queueFamilyIndices;
	}
	else
	{
		// True for most hardware. Queue Family is the same for Graphics & Presentation queue
		createInfo.imageSharingMode      = VK_SHARING_MODE_EXCLUSIVE;
		createInfo.queueFamilyIndexCount = 0;     // Optional
		createInfo.pQueueFamilyIndices   = nullptr; // Optional
	}

	createInfo.preTransform   = swapChainSupport.capabilities.currentTransform;

	// This is a special case of blending between multiple windows which we don't currently need. It does not mean alpha blending is disabled in general. 
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR; 
	createInfo.presentMode    = presentMode;

	// Clip obscured pixels
	createInfo.clipped = VK_TRUE;

	// If we recreate the swapchain, this needs to point to the old one.
	createInfo.oldSwapchain = VK_NULL_HANDLE;

	if (vkCreateSwapchainKHR(m_VulkanDevice, &createInfo, nullptr, &m_Swapchain) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create swap chain!");
	}

	vkGetSwapchainImagesKHR(m_VulkanDevice, m_Swapchain, &imageCount, nullptr);
	m_SwapchainImages.resize(imageCount);
	vkGetSwapchainImagesKHR(m_VulkanDevice, m_Swapchain, &imageCount, m_SwapchainImages.data());

	m_SwapchainImageFormat = surfaceFormat.format;
	m_SwapchainExtent = extent;

#if defined(_DEBUG)
	std::cout << "Swapchain succesfully created!\n" << std::endl;
#endif
}

void TriangleApp::CleanupSwapchain()
{
	for (size_t i = 0; i < m_Framebuffers.size(); i++) {
		vkDestroyFramebuffer(m_VulkanDevice, m_Framebuffers[i], nullptr);
	}

	for (size_t i = 0; i < m_SwapchainImageViews.size(); i++) {
		vkDestroyImageView(m_VulkanDevice, m_SwapchainImageViews[i], nullptr);
	}

	vkDestroySwapchainKHR(m_VulkanDevice, m_Swapchain, nullptr);
}

void TriangleApp::RecreateSwapChain()
{
#if defined(_DEBUG)
	std::cout << "\n -- Swapchain invalidated! --\n" << std::endl;
#endif

	// Need to wait for queued GPU commands to finish
	vkDeviceWaitIdle(m_VulkanDevice);

	CleanupSwapchain();

	// Recreate resources that depended on the swap chain
	CreateSwapChain();
	CreateImageViews();
	CreateFrameBuffers();

#if defined(_DEBUG)
	std::cout << "\n -- Swapchain recreated! --\n" << std::endl;
#endif	
}

void TriangleApp::CreateImageViews()
{
	m_SwapchainImageViews.resize(m_SwapchainImages.size());

	for (size_t i = 0; i < m_SwapchainImageViews.size(); i++)
	{
		VkImageViewCreateInfo createInfo {};
		createInfo.sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		createInfo.image    = m_SwapchainImages[i];
		createInfo.format   = m_SwapchainImageFormat;

		createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

		createInfo.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
		createInfo.subresourceRange.baseMipLevel   = 0;
		createInfo.subresourceRange.levelCount     = 1;
		createInfo.subresourceRange.baseArrayLayer = 0;
		createInfo.subresourceRange.layerCount     = 1;

		if (vkCreateImageView(m_VulkanDevice, &createInfo, nullptr, &m_SwapchainImageViews[i]) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create image view!");
		}
	}
}
void TriangleApp::CreateRenderPass()
{
	VkAttachmentDescription colorAttachment{};
	colorAttachment.format         = m_SwapchainImageFormat;
	colorAttachment.samples        = VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	
	VkAttachmentReference attachmentReference{};
	attachmentReference.attachment = 0;
	attachmentReference.layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass{};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &attachmentReference;

	VkRenderPassCreateInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = 1;
	renderPassInfo.pAttachments = &colorAttachment;
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;

	VkSubpassDependency dependency{};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	renderPassInfo.dependencyCount = 1;
	renderPassInfo.pDependencies = &dependency;

	if (vkCreateRenderPass(m_VulkanDevice, &renderPassInfo, nullptr, &m_RenderPass) != VK_SUCCESS) 
	{
		throw std::runtime_error("failed to create render pass!");
	}
}


void TriangleApp::CreateGraphicsPipeline()
{
	VkShaderModule vertexShaderModule     = CreateShaderModule("shaders/vert.spv");
	VkShaderModule fragmentShaderModule   = CreateShaderModule("shaders/frag.spv");


	VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
	vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertShaderStageInfo.module = vertexShaderModule;
	vertShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
	fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragShaderStageInfo.module = fragmentShaderModule;
	fragShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

	VkPipelineVertexInputStateCreateInfo vertexInputInfo{};

	auto bindingDescription = Vertex::GetBindingDescription();
	auto attributeDescriptions = Vertex::GetAttributeDescriptions();

	vertexInputInfo.vertexBindingDescriptionCount = 1; // single binding descriptor for now
	vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size()); // with 2 attributes
	vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
	vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputInfo.vertexBindingDescriptionCount = 0;
	vertexInputInfo.vertexAttributeDescriptionCount = 0;

	VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssembly.primitiveRestartEnable = VK_FALSE;

	VkPipelineViewportStateCreateInfo viewportState{};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.scissorCount = 1;

	VkPipelineRasterizationStateCreateInfo rasterizer{};
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.depthClampEnable = VK_FALSE;
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizer.lineWidth = 1.0f;
	rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
	rasterizer.depthBiasEnable = VK_FALSE;

	VkPipelineMultisampleStateCreateInfo multisampling{};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable = VK_FALSE;
	multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

	VkPipelineColorBlendAttachmentState colorBlendAttachment{};
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable = VK_FALSE;

	VkPipelineColorBlendStateCreateInfo colorBlending{};
	colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.logicOp = VK_LOGIC_OP_COPY;
	colorBlending.attachmentCount = 1;
	colorBlending.pAttachments = &colorBlendAttachment;
	colorBlending.blendConstants[0] = 0.0f;
	colorBlending.blendConstants[1] = 0.0f;
	colorBlending.blendConstants[2] = 0.0f;
	colorBlending.blendConstants[3] = 0.0f;

	std::vector<VkDynamicState> dynamicStates = 
	{
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR
	};

	VkPipelineDynamicStateCreateInfo dynamicState{};
	dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
	dynamicState.pDynamicStates = dynamicStates.data();

	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 0;
	pipelineLayoutInfo.pushConstantRangeCount = 0;

	if (vkCreatePipelineLayout(m_VulkanDevice, &pipelineLayoutInfo, nullptr, &m_PipelineLayout) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create pipeline layout!");
	}

	VkGraphicsPipelineCreateInfo pipelineInfo{};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stageCount = 2;
	pipelineInfo.pStages = shaderStages;
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pMultisampleState = &multisampling;
	pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.pDynamicState = &dynamicState;
	pipelineInfo.layout = m_PipelineLayout;
	pipelineInfo.renderPass = m_RenderPass;
	pipelineInfo.subpass = 0;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

	if (vkCreateGraphicsPipelines(m_VulkanDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_GraphicsPipeline) != VK_SUCCESS) 
	{
		throw std::runtime_error("failed to create graphics pipeline!");
	}

	vkDestroyShaderModule(m_VulkanDevice, vertexShaderModule, nullptr);
	vkDestroyShaderModule(m_VulkanDevice, fragmentShaderModule, nullptr);

#if defined(_DEBUG)
	std::cout << "\nGraphics Pipeline created succesfully!" << std::endl;
#endif
}

void TriangleApp::CreateFrameBuffers()
{
	m_Framebuffers.resize(m_SwapchainImages.size());

	for (size_t i = 0; i < m_SwapchainImageViews.size(); i++)
	{
		VkImageView attachments[] = 
		{
			m_SwapchainImageViews[i]
		};

		VkFramebufferCreateInfo framebufferInfo{};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = m_RenderPass;
		framebufferInfo.attachmentCount = 1;
		framebufferInfo.pAttachments = attachments;
		framebufferInfo.width = m_SwapchainExtent.width;
		framebufferInfo.height = m_SwapchainExtent.height;
		framebufferInfo.layers = 1;

		if (vkCreateFramebuffer(m_VulkanDevice, &framebufferInfo, nullptr, &m_Framebuffers[i]) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create framebuffer!");
		}
	}

#if defined(_DEBUG)
	std::cout << "Framebuffers setup!" << " Resolution: {" << m_SwapchainExtent.width << ", " << m_SwapchainExtent.height << "}" << std::endl;
#endif
}

void TriangleApp::CreateCommandPool()
{
	VulkanQueueFamilyIndices queueFamilyIndices = FindVulkanQueueFamilies(m_PhysicalDevice);

	VkCommandPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	poolInfo.queueFamilyIndex = queueFamilyIndices.m_GraphicsFamily.value();

	if (vkCreateCommandPool(m_VulkanDevice, &poolInfo, nullptr, &m_CommandPool) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create command pool!");
	}
}

void TriangleApp::CreateCommandBuffers()
{
	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = m_CommandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

	m_CommandBuffers.resize(MAX_FRAMES_IN_FLIGHT);
	allocInfo.commandBufferCount = (uint32_t) m_CommandBuffers.size();

	if (vkAllocateCommandBuffers(m_VulkanDevice, &allocInfo, m_CommandBuffers.data()) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to allocate command buffers!");
	}

#if defined(_DEBUG)
	std::cout << "Command Pool & Buffers created!" << std::endl;
#endif
}

void TriangleApp::RecordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex)
{
	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = 0; // Optional
	beginInfo.pInheritanceInfo = nullptr; // Optional

	if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) 
	{
		throw std::runtime_error("failed to begin recording command buffer!");
	}

	VkRenderPassBeginInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = m_RenderPass;
	renderPassInfo.framebuffer = m_Framebuffers[imageIndex];
	renderPassInfo.renderArea.offset = { 0, 0 };
	renderPassInfo.renderArea.extent = m_SwapchainExtent;

	VkClearValue clearColor = { {{0.0f, 0.0f, 0.0f, 1.0f}} };

	renderPassInfo.clearValueCount = 1;
	renderPassInfo.pClearValues = &clearColor;

	vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_GraphicsPipeline);

	VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = static_cast<float>(m_SwapchainExtent.width);
	viewport.height = static_cast<float>(m_SwapchainExtent.height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

	VkRect2D scissor{};
	scissor.offset = { 0, 0 };
	scissor.extent = m_SwapchainExtent;
	vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

	vkCmdDraw(commandBuffer, 3, 1, 0, 0);

	vkCmdEndRenderPass(commandBuffer);

	if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) 
	{
		throw std::runtime_error("failed to record command buffer!");
	}
}

void TriangleApp::CreateSyncObjects()
{
	m_ImageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
	m_RenderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
	m_InFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

	VkSemaphoreCreateInfo semaphoreInfo{};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkFenceCreateInfo fenceInfo{};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		if (vkCreateSemaphore(m_VulkanDevice, &semaphoreInfo, nullptr, &m_ImageAvailableSemaphores[i]) != VK_SUCCESS ||
			vkCreateSemaphore(m_VulkanDevice, &semaphoreInfo, nullptr, &m_RenderFinishedSemaphores[i]) != VK_SUCCESS ||
			vkCreateFence(m_VulkanDevice,     &fenceInfo,     nullptr, &m_InFlightFences[i])           != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create semaphores!");
		}
	}

}

void TriangleApp::DrawFrame()
{
	vkWaitForFences(m_VulkanDevice, 1, &m_InFlightFences[m_FrameIndex], VK_TRUE, UINT64_MAX);

	uint32_t imageIndex;
	VkResult result = vkAcquireNextImageKHR(m_VulkanDevice, m_Swapchain, UINT64_MAX, m_ImageAvailableSemaphores[m_FrameIndex], VK_NULL_HANDLE, &imageIndex);

	// Need to recreate swapchain and discard this frame
	if (result == VK_ERROR_OUT_OF_DATE_KHR) {
		RecreateSwapChain();
		return;
	}
	else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
		throw std::runtime_error("failed to acquire swap chain image!");
	}

	// Only reset the fence if we are clear to submit work
	vkResetFences(m_VulkanDevice, 1, &m_InFlightFences[m_FrameIndex]);

	vkResetCommandBuffer(m_CommandBuffers[m_FrameIndex], 0);

	RecordCommandBuffer(m_CommandBuffers[m_FrameIndex], imageIndex);

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	VkSemaphore waitSemaphores[] = { m_ImageAvailableSemaphores[m_FrameIndex]};
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = waitSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &m_CommandBuffers[m_FrameIndex];

	VkSemaphore signalSemaphores[] = { m_RenderFinishedSemaphores[m_FrameIndex]};
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;

	if (vkQueueSubmit(m_GraphicsQueue, 1, &submitInfo, m_InFlightFences[m_FrameIndex]) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to submit draw command buffer!");
	}

	VkPresentInfoKHR presentInfo{};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = signalSemaphores;
	presentInfo.pResults = nullptr; // Optional

	VkSwapchainKHR swapchains[] = { m_Swapchain };
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapchains;
	presentInfo.pImageIndices = &imageIndex;

	result = vkQueuePresentKHR(m_PresentQueue, &presentInfo);

	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
		RecreateSwapChain();
	}
	else if (result != VK_SUCCESS) {
		throw std::runtime_error("failed to present swap chain image!");
	}

	m_FrameIndex = (m_FrameIndex + 1) % MAX_FRAMES_IN_FLIGHT;
}

void TriangleApp::InitVulkan()
{
#if defined(_DEBUG)
	std::cout << " ---- Vulkan Initializing! ---- \n" << std::endl;
#endif
	CreateVulkanInstance();
	SetupDebugMessenger();
	CreateVulkanSurface();
	SelectPhysicalDevice();
	CreateLogicalDevice();
	CreateSwapChain();
	CreateImageViews();
	CreateRenderPass();
	CreateGraphicsPipeline();
	CreateFrameBuffers();
	CreateCommandPool();
	CreateCommandBuffers();
	CreateSyncObjects();
#if defined(_DEBUG)
	std::cout << "\n ---- Vulkan fully initialized! ----" << std::endl;
#endif
}

void TriangleApp::MainLoop()
{
	SDL_Event sdl_event;
	bool      shouldQuit   = false;

	while (!shouldQuit)
	{
		while (SDL_PollEvent(&sdl_event))
		{
			shouldQuit = sdl_event.type == SDL_QUIT || sdl_event.type == SDL_WINDOWEVENT && sdl_event.window.event == SDL_WINDOWEVENT_CLOSE;
		}
		DrawFrame();
	}

	glm::mat4 matrix;
	glm::vec4 vec;
	auto test = matrix * vec;

	vkDeviceWaitIdle(m_VulkanDevice);
}

void TriangleApp::CleanupApp()
{
	CleanupSwapchain();

	vkDestroyPipeline(m_VulkanDevice, m_GraphicsPipeline, nullptr);
	vkDestroyPipelineLayout(m_VulkanDevice, m_PipelineLayout, nullptr);

	vkDestroyRenderPass(m_VulkanDevice, m_RenderPass, nullptr);

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		vkDestroySemaphore(m_VulkanDevice, m_ImageAvailableSemaphores[i], nullptr);
		vkDestroySemaphore(m_VulkanDevice, m_RenderFinishedSemaphores[i], nullptr);
		vkDestroyFence(m_VulkanDevice, m_InFlightFences[i], nullptr);
	}

	vkDestroyCommandPool(m_VulkanDevice, m_CommandPool, nullptr);

	vkDestroyDevice(m_VulkanDevice, nullptr);

	if (g_EnableValidationLayers)
	{
		DestroyDebugUtilsMessengerEXT(m_VulkanInstance, m_DebugMessenger, nullptr);
	}

	vkDestroySurfaceKHR(m_VulkanInstance, m_VulkanSurface, nullptr);
	vkDestroyInstance(m_VulkanInstance, nullptr);

	SDL_DestroyWindow(m_Window);
	SDL_Quit();
}