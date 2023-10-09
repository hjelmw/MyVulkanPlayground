#include "ShadowPass.hpp"

#define SHADOWMAP_FORMAT     VK_FORMAT_D16_UNORM
#define SHADOWMAP_RESOLUTION 1024
#define SHADOWMAP_BIAS       1.25f

namespace NVulkanEngine
{
	void CShadowPass::InitPass(CGraphicsContext* context)
	{
		//s_ShadowAttachment = CreateAttachment(
		//	context,
		//	SHADOWMAP_FORMAT,
		//	VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
		//	VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL,
		//	SHADOWMAP_RESOLUTION,
		//	SHADOWMAP_RESOLUTION);
	}

	void CShadowPass::UpdateShadowBuffers(CGraphicsContext* context)
	{

	}

	void CShadowPass::Draw(CGraphicsContext* context, VkCommandBuffer commandBuffer)
	{

	}

	void CShadowPass::CleanupPass(CGraphicsContext* context)
	{

	}
};