#pragma once


#include <array>
#include <string>

#include <GraphicsContext.hpp>
#include <VulkanGraphicsEngineUtils.hpp>
#include <BindlessBuffer.hpp>

/*
	Attachment manager holds all the possible render to textures and keeps track of their states for you
*/

enum class EResourceIndices : uint32_t
{
	Positions          = 0,
	Normals            = 1,
	Albedo             = 2,
	Depth              = 3,
	ShadowMap          = 4,
	SceneColor         = 5,
	AtmosphericsSkyBox = 6,
	Count              = 7
};

enum class EBufferIndices : uint32_t
{
	Geometry     = 0,
	Shadows      = 1,
	Atmospherics = 2,
	Terrain      = 3,
	DebugLines   = 4,
	Count        = 5,
};

namespace NVulkanEngine
{
	class CResourceManager
	{
	public:
		CResourceManager(VkInstance vulkanInstance);
		~CResourceManager() = default;

		SRenderResource AddRenderResource(
			CGraphicsContext*     context,
			const std::string     debugName,
			EResourceIndices    attachmentIndex,
			VkSampler             sampler,
			VkShaderStageFlagBits shaderStageUsageFlags,
			VkFormat              format,
			VkImageUsageFlags     usage,
			VkImageLayout         imageLayout,
			uint32_t              width,
			uint32_t              height);

		SUniformBufferResource AddUniformBuffer(
			CGraphicsContext* context,
			const std::string& debugName,
			EBufferIndices     uniformBufferIndex,
			VkDeviceSize       uniformBufferSize);

		// Returns the attachment if it has been created. You cannot modify this
		const SRenderResource GetRenderResource(EResourceIndices renderIndex);

		// Returns the buffer if it has been created. You cannot modify this
		const SUniformBufferResource GetBufferResource(EBufferIndices bufferIndex);
		
		VkDescriptorSetLayout GetBindlessDescriptorLayout();

		const std::array<SRenderResource, (uint32_t)EResourceIndices::Count> GetRenderResources();
		const std::array<SUniformBufferResource, (uint32_t)EBufferIndices::Count> GetBufferResources();

		// Returns the attachment in state ready for rendering
		SRenderResource TransitionResource(VkCommandBuffer commandBuffer, EResourceIndices index,VkAttachmentLoadOp loadOperation, VkImageLayout wantedState);

		void Cleanup(CGraphicsContext* context);
	private:
		std::array<SRenderResource, (uint32_t)EResourceIndices::Count> m_RenderResources = {};
		std::array<SUniformBufferResource, (uint32_t)EBufferIndices::Count>   m_BufferResources = {};

		CBindlessBuffer* m_BindlessBuffer = nullptr;

		// To mark attachments with debug names
		PFN_vkSetDebugUtilsObjectNameEXT m_VkSetDebugUtilsObjectNameEXT = nullptr;
	};
};