#pragma once

#include <vector>
#include <glm/glm.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <GraphicsContext.hpp>

/*
	Immediate mode render debug lines.
*/

namespace NVulkanEngine
{
	struct SDebugVertexLine
	{
		glm::vec3 m_Position = glm::vec3(0.0f, 0.0f, 0.0f);
		glm::vec3 m_Color    = glm::vec3(0.0f, 0.0f, 0.0f);
	};

	class CDebugManager
	{
	public:
		CDebugManager() = default;
		~CDebugManager() = default;

		void DrawDebugLine(glm::vec3 from, glm::vec3 to, glm::vec3 color);
		void DrawDebugAABB(glm::vec3 from, glm::vec3 to, glm::vec3 color);

		VkBuffer GetDebugLinesVertexBuffer();
		const uint32_t GetNumDebugLines();

		void Update(CGraphicsContext* context);
		void Cleanup(CGraphicsContext* context);
	private:
		std::vector<SDebugVertexLine> m_DebugVertexLinesAddList    = {};
		std::vector<SDebugVertexLine> m_DebugVertexLinesRenderList = {};

		VkBuffer       m_DebugVertexBuffer        = VK_NULL_HANDLE;
		VkBuffer       m_StagingVertexBuffer      = VK_NULL_HANDLE;

		VkDeviceMemory m_DebugVertexBufferMemory   = VK_NULL_HANDLE;
		VkDeviceMemory m_StagingVertexBufferMemory = VK_NULL_HANDLE;

		uint32_t       m_VertexBuferSize = 0;
	};
};