#include <vulkan/vulkan.h>

#include <Core/GraphicsContext.hpp>
#include <stbi/stb_image.h>
#include <string>

/*
	An image loaded from disk that a shader can read from.
	render targets or textures created by the graphics engine do not use this class currently
*/

namespace NVulkanEngine
{
	class CTexture
	{
	public:
		CTexture()  = default;
		~CTexture() = default;

		void        SetGenerateMipmaps(bool generate) { m_GenerateMipmaps = generate; };
		void        CreateTexture(SRuntimeContext* context, std::string textureFilepath, VkFormat format);

		VkImageView GetTextureImageView() { return m_TextureImageView ? m_TextureImageView : VK_NULL_HANDLE; };
		VkFormat    GetTextureFormat()    { return m_TextureFormat; }
		uint32_t    GetMipmapLevels()     { return m_GenerateMipmaps ? m_MipLevels : 1; }
		
		void DestroyTexture(SRuntimeContext* context);
	private:
		void CreateTextureImage(SRuntimeContext* context, stbi_uc* pixels, uint32_t texWidth, uint32_t texHeight, VkFormat format);
		void CreateTextureImageView(SRuntimeContext* context);

		void GenerateMipmaps(SRuntimeContext* context, VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels);

		bool                  m_GenerateMipmaps    = false;
		uint32_t              m_MipLevels          = 1;
		VkFormat              m_TextureFormat      = VK_FORMAT_UNDEFINED;

		// Texture image & view
		VkImage               m_TextureImage       = VK_NULL_HANDLE;
		VkDeviceMemory        m_TextureImageMemory = VK_NULL_HANDLE;
		VkImageView	          m_TextureImageView   = VK_NULL_HANDLE;
	};

}