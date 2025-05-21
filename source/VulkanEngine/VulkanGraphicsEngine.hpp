#pragma once

// 3rd party
#include <vulkan/vulkan.h>

#include <glm/glm.hpp>

#include <GLFW/glfw3.h>

#include <VulkanGraphicsEngineUtils.hpp>

#include <DrawNodes/DrawNode.hpp>
#include <DrawNodes/Utils/Pipeline.hpp>

#include <GraphicsContext.hpp>
#include <Swapchain.hpp>

#include <Managers/LightManager.hpp> // Need ELightType in header
#include <Managers/DebugManager.hpp>

#include <BindlessBuffer.hpp>

class CModelManager;
class CInputManager;

/*
    The main engine whatchamacallit. It has most of the rendering code right now
*/

namespace NVulkanEngine
{
    class CVulkanGraphicsEngine
    {
    public:
        CVulkanGraphicsEngine()  = default;
        ~CVulkanGraphicsEngine() = default;

        void Initialize();

        void AddModelFilepath(const std::string& modelpath);
        void SetModelTexture(const std::string& texturepath);
        void SetModelPosition(float x, float y, float z);
        void SetModelRotation(float x, float y, float z);
        void SetModelScaling(float x, float y, float z);
        void PushModel();

        void AddLightSource(ELightType lightType);
        void SetLightPosition(float x, float y, float z);
        void SetLightDirection(float x, float y, float z);
        void SetLightIntensity(float intensity);
        void PushLight();

        void CreateScene();

        void DrawFrame();
        bool IsRunning();
        void Cleanup();

    private:
        // Initialize
        void InitWindow();
        void InitVulkan();
        void InitImGui();
        void InitSwapchain();
        void InitCamera();
        void CreateGraphicsContext();

        // Vulkan init
        void CreateVulkanInstance();
        void SetupDebugUtils();
        void CreateVulkanSurface();
        void SelectPhysicalDevice();
        void CreateLogicalDevice();
        void CreateCommandPool();
        void CreateCommandBuffers();
        void CreateSyncObjects();
        void CreateSamplers();
        void CreateResources();

        // Create scene
        void CreateModels();
        void InitDrawNodes();
        void InitManagers();

        void CleanupManagers();
        void CleanupDrawNodes();
        void CleanupSyncObjects();
        void CleanupVulkan();
        void CleanupWindow();
        void CleanupImGui();

        void ResizeFrame();

        bool CheckDeviceExtensionSupport(VkPhysicalDevice device, const std::vector<const char*> deviceExtensions);
        bool IsDeviceSuitable(VkPhysicalDevice device, VkSurfaceKHR surface, const std::vector<const char*> deviceExtensions);

        void     PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
        VkResult CreateDebugUtilsMessengerEXT(const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger);
        VkResult DestroyDebugUtilsMessengerEXT(VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator);

        void     ProcessGLFWKeyboardInputs(GLFWwindow* window, int key, int scancode, int action, int mods);
        void     ProcessGLFWMouseInput(GLFWwindow* window, int button, int action, int mods);
        void     ResizeGLFWFrame(GLFWwindow* window, int newWidth, int newHeight);

        void     RecordDrawNodes(VkCommandBuffer commandBuffer);
        void	 RenderImGuiDrawData(uint32_t imageIndex);
        void     DoImGuiViewport();

        bool m_IsRunning = false;
        bool m_NeedsResize = false;

        // Draw nodes specifies render order
        std::array<CDrawNode*, (uint32_t)EDrawNodes::Count> m_DrawNodes               = {};

        CGraphicsContext* m_Context   = nullptr;
        CSwapchain*       m_Swapchain = nullptr;

        // Managers
        CInputManager*                      m_InputManager             = nullptr;
        CModelManager*                      m_ModelManager             = nullptr;
        CLightManager*                      m_LightManager             = nullptr;
        CDebugManager*                      m_DebugManager             = nullptr;
        CPipelineManager*                   m_PipelineManager          = nullptr;
        CResourceManager*                   m_ResourceManager          = nullptr;

        /* Vulkan Primitives */
        // Device
        VkInstance			                m_VulkanInstance           = VK_NULL_HANDLE;
        VkPhysicalDevice	                m_PhysicalDevice           = VK_NULL_HANDLE;
        VkDevice			                m_VulkanDevice             = VK_NULL_HANDLE;
        VkSurfaceKHR		                m_VulkanSurface            = VK_NULL_HANDLE;

        // Queues
        SVulkanQueueFamilyIndices           m_QueueFamilies            = {};
        VkQueue				                m_GraphicsQueue            = VK_NULL_HANDLE;
        VkQueue				                m_PresentQueue             = VK_NULL_HANDLE;

        // CommandBuffer
        VkCommandPool                       m_CommandPool              = VK_NULL_HANDLE;
        std::vector<VkCommandBuffer>        m_CommandBuffers           = {};

        VkSampler                           m_LinearClamp              = VK_NULL_HANDLE;
        VkSampler                           m_LinearRepeat             = VK_NULL_HANDLE;

        // Changes on window resize
        VkExtent2D                          m_NewRenderResolution      = { 0, 0 };

        // Frame Index
        uint32_t                            m_FrameIndex               = 0;

        // Synchronization
        std::vector<VkSemaphore>            m_ImageAvailableSemaphores = {};
        std::vector<VkSemaphore>            m_RenderFinishedSemaphores = {};
        std::vector<VkFence>                m_InFlightFences           = {};

        CBindlessBuffer*                    m_BindlessBuffer            = nullptr;

        // Misc
        VkDebugUtilsMessengerEXT            m_DebugMessenger           = VK_NULL_HANDLE;
        GLFWwindow*					        m_Window                   = nullptr;

        VkDescriptorPool                    m_ImGuiDescriptorPool      = VK_NULL_HANDLE;

        CPipeline*                          m_ImGuiPipeline            = VK_NULL_HANDLE;


    };
}