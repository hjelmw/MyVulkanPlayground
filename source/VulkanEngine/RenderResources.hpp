#pragma once

#include <concepts>

#include <vulkan/vulkan.h>

/*
	The resources created by the engine.
	Add the definition here and then create it in CVulkanGraphicsEngine::CreateResources()
*/

// Here I used a C++20 concept to ensure that you add these members!
template<class TRenderResource>
concept CRenderResourceTag = requires
{
	{ TRenderResource::m_DebugName }    -> std::convertible_to<const char*>;
	{ TRenderResource::m_Format }       -> std::convertible_to<VkFormat>;
	{ TRenderResource::m_Usage }        -> std::convertible_to<VkImageUsageFlags>;
	{ TRenderResource::m_InitialState } -> std::convertible_to<VkImageLayout>;
};

namespace NRenderResources
{
	struct SGBufferPositions
	{
		static constexpr const char*        m_DebugName    = "GBuffer - Positions";
		static constexpr VkFormat           m_Format       = VK_FORMAT_R16G16B16A16_SFLOAT;
		static constexpr VkImageUsageFlags  m_Usage        = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		static constexpr VkImageLayout      m_InitialState = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	};

	struct SGBufferNormals
	{
		static constexpr const char*        m_DebugName    = "GBuffer - Normals";
		static constexpr VkFormat           m_Format       = VK_FORMAT_R16G16B16A16_SFLOAT;
		static constexpr VkImageUsageFlags  m_Usage        = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		static constexpr VkImageLayout      m_InitialState = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	};

	struct SGBufferAlbedo
	{
		static constexpr const char*        m_DebugName    = "GBuffer - Albedo";
		static constexpr VkFormat           m_Format       = VK_FORMAT_R8G8B8A8_UNORM;
		static constexpr VkImageUsageFlags  m_Usage        = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		static constexpr VkImageLayout      m_InitialState = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	};

	struct SGBufferDepth
	{
		static constexpr const char*        m_DebugName    = "GBuffer - Depth";
		static constexpr VkFormat           m_Format       = VK_FORMAT_UNDEFINED; // Chosen at runtime, pass a format override to Add
		static constexpr VkImageUsageFlags  m_Usage        = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		static constexpr VkImageLayout      m_InitialState = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
	};

	struct SSunShadowMap
	{
		static constexpr const char*        m_DebugName    = "Shadow Map";
		static constexpr VkFormat           m_Format       = VK_FORMAT_D32_SFLOAT;
		static constexpr VkImageUsageFlags  m_Usage        = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		static constexpr VkImageLayout      m_InitialState = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	};

	struct SAtmosphericsSkyBox
	{
		static constexpr const char*        m_DebugName    = "Atmospherics SkyBox";
		static constexpr VkFormat           m_Format       = VK_FORMAT_R8G8B8A8_UNORM;
		static constexpr VkImageUsageFlags  m_Usage        = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		static constexpr VkImageLayout      m_InitialState = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	};

	struct SSceneColor
	{
		static constexpr const char*        m_DebugName    = "Scene Color";
		static constexpr VkFormat           m_Format       = VK_FORMAT_R8G8B8A8_UNORM;
		static constexpr VkImageUsageFlags  m_Usage        = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT; // Storage bit allows compute shaders to write
		static constexpr VkImageLayout      m_InitialState = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	};
}