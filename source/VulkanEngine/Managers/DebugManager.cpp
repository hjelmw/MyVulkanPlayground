#include "DebugManager.hpp"
#include <VulkanGraphicsEngineUtils.hpp>

namespace NVulkanEngine
{
	void CDebugManager::DrawDebugLine(glm::vec3 from, glm::vec3 to, glm::vec3 color)
	{
		SDebugVertexLine debugLineFrom{};
		debugLineFrom.m_Position = from;
		debugLineFrom.m_Color    = color;
	
		SDebugVertexLine debugLineTo{};
		debugLineTo.m_Position = to; 
		debugLineTo.m_Color    = color;

		m_DebugVertexLinesAddList.push_back(debugLineFrom);
		m_DebugVertexLinesAddList.push_back(debugLineTo);
	}

	void CDebugManager::DrawDebugAABB(glm::AABB boundingBox, glm::vec3 color)
	{
		glm::vec3 min = boundingBox.getMin();
		glm::vec3 max = boundingBox.getMax();

		DrawDebugAABB(min, max, color);
	}

	void CDebugManager::DrawDebugAABB(glm::vec3 min, glm::vec3 max, glm::vec3 color)
	{
		// Top part of box
		m_DebugVertexLinesAddList.push_back({ glm::vec3(min.x, max.y, min.z), color });
		m_DebugVertexLinesAddList.push_back({ glm::vec3(max.x, max.y, min.z), color });

		m_DebugVertexLinesAddList.push_back({ glm::vec3(max.x, max.y, min.z), color });
		m_DebugVertexLinesAddList.push_back({ glm::vec3(max.x, max.y, max.z), color });

		m_DebugVertexLinesAddList.push_back({ glm::vec3(max.x, max.y, max.z), color });
		m_DebugVertexLinesAddList.push_back({ glm::vec3(min.x, max.y, max.z), color });

		m_DebugVertexLinesAddList.push_back({ glm::vec3(min.x, max.y, max.z), color });
		m_DebugVertexLinesAddList.push_back({ glm::vec3(min.x, max.y, min.z), color });

		// Bottom part of box
		m_DebugVertexLinesAddList.push_back({ glm::vec3(min.x, min.y, min.z), color });
		m_DebugVertexLinesAddList.push_back({ glm::vec3(max.x, min.y, min.z), color });

		m_DebugVertexLinesAddList.push_back({ glm::vec3(max.x, min.y, min.z), color });
		m_DebugVertexLinesAddList.push_back({ glm::vec3(max.x, min.y, max.z), color });

		m_DebugVertexLinesAddList.push_back({ glm::vec3(max.x, min.y, max.z), color });
		m_DebugVertexLinesAddList.push_back({ glm::vec3(min.x, min.y, max.z), color });

		m_DebugVertexLinesAddList.push_back({ glm::vec3(min.x, min.y, max.z), color });
		m_DebugVertexLinesAddList.push_back({ glm::vec3(min.x, min.y, min.z), color });

		// Sides of box
		m_DebugVertexLinesAddList.push_back({ glm::vec3(min.x, min.y, min.z), color });
		m_DebugVertexLinesAddList.push_back({ glm::vec3(min.x, max.y, min.z), color });

		m_DebugVertexLinesAddList.push_back({ glm::vec3(max.x, min.y, min.z), color });
		m_DebugVertexLinesAddList.push_back({ glm::vec3(max.x, max.y, min.z), color });

		m_DebugVertexLinesAddList.push_back({ glm::vec3(min.x, min.y, max.z), color });
		m_DebugVertexLinesAddList.push_back({ glm::vec3(min.x, max.y, max.z), color });

		m_DebugVertexLinesAddList.push_back({ glm::vec3(max.x, min.y, max.z), color });
		m_DebugVertexLinesAddList.push_back({ glm::vec3(max.x, max.y, max.z), color });
	}

	void CDebugManager::DrawDebugOBB(glm::AABB aabb, glm::mat4 transformMatrix, glm::vec3 color)
	{

		glm::vec4 AABBMinCorner = glm::vec4(aabb.getMin(), 1.0f);
		glm::vec4 AABBMaxCorner = glm::vec4(aabb.getMax(), 1.0f);

		const float diffMaxMinX = AABBMaxCorner.x - AABBMinCorner.x;
		const float diffMaxMinY = AABBMaxCorner.y - AABBMinCorner.y;
		const float diffMinMaxX = AABBMinCorner.x - AABBMaxCorner.x;
		const float diffMinMaxY = AABBMinCorner.y - AABBMaxCorner.y;

		std::array<glm::vec3, 8> orientedCorners = {};
		orientedCorners[0] = transformMatrix *  AABBMinCorner;
		orientedCorners[1] = transformMatrix * (AABBMinCorner + glm::vec4(diffMaxMinX, 0.0f,        0.0f, 1.0f));
		orientedCorners[2] = transformMatrix * (AABBMinCorner + glm::vec4(diffMaxMinX, diffMaxMinY, 0.0f, 1.0f));
		orientedCorners[3] = transformMatrix * (AABBMinCorner + glm::vec4(0.0f,        diffMaxMinY, 0.0f, 1.0f));

		orientedCorners[4] = transformMatrix *  AABBMaxCorner;
		orientedCorners[5] = transformMatrix * (AABBMaxCorner + glm::vec4(diffMinMaxX, 0.0f,        0.0f, 1.0f));
		orientedCorners[6] = transformMatrix * (AABBMaxCorner + glm::vec4(diffMinMaxX, diffMinMaxY, 0.0f, 1.0f));
		orientedCorners[7] = transformMatrix * (AABBMaxCorner + glm::vec4(0.0f,        diffMinMaxY, 0.0f, 1.0f));

		m_DebugVertexLinesAddList.push_back({ orientedCorners[0], color });
		m_DebugVertexLinesAddList.push_back({ orientedCorners[1], color });
		m_DebugVertexLinesAddList.push_back({ orientedCorners[0], color });
		m_DebugVertexLinesAddList.push_back({ orientedCorners[3], color });
		m_DebugVertexLinesAddList.push_back({ orientedCorners[2], color });
		m_DebugVertexLinesAddList.push_back({ orientedCorners[1], color });
		m_DebugVertexLinesAddList.push_back({ orientedCorners[2], color });
		m_DebugVertexLinesAddList.push_back({ orientedCorners[3], color });

		m_DebugVertexLinesAddList.push_back({ orientedCorners[4], color });
		m_DebugVertexLinesAddList.push_back({ orientedCorners[5], color });
		m_DebugVertexLinesAddList.push_back({ orientedCorners[4], color });
		m_DebugVertexLinesAddList.push_back({ orientedCorners[7], color });
		m_DebugVertexLinesAddList.push_back({ orientedCorners[6], color });
		m_DebugVertexLinesAddList.push_back({ orientedCorners[5], color });
		m_DebugVertexLinesAddList.push_back({ orientedCorners[6], color });
		m_DebugVertexLinesAddList.push_back({ orientedCorners[7], color });

		m_DebugVertexLinesAddList.push_back({ orientedCorners[0], color });
		m_DebugVertexLinesAddList.push_back({ orientedCorners[6], color });
		m_DebugVertexLinesAddList.push_back({ orientedCorners[1], color });
		m_DebugVertexLinesAddList.push_back({ orientedCorners[7], color });
		m_DebugVertexLinesAddList.push_back({ orientedCorners[2], color });
		m_DebugVertexLinesAddList.push_back({ orientedCorners[4], color });
		m_DebugVertexLinesAddList.push_back({ orientedCorners[3], color });
		m_DebugVertexLinesAddList.push_back({ orientedCorners[5], color });

	}


	VkBuffer CDebugManager::GetDebugLinesVertexBuffer()
	{
		return m_DebugVertexBuffer;
	}

	const uint32_t CDebugManager::GetNumDebugLines()
	{
		return (uint32_t)m_DebugVertexLinesRenderList.size();
	}

	void CDebugManager::Update(CGraphicsContext* context)
	{
		if (m_DebugVertexLinesAddList.empty())
			return;

		// Just init it to something to begin with
		VkMemoryRequirements memoryRequirements;
		memoryRequirements.size = 0;
		memoryRequirements.alignment = 0;

		uint32_t currentVertexBufferSize = 0;
		if (m_DebugVertexBuffer)
		{
			vkGetBufferMemoryRequirements(context->GetLogicalDevice(), m_DebugVertexBuffer, &memoryRequirements);
			currentVertexBufferSize = (uint32_t)memoryRequirements.size;
		}

		const uint32_t neededVertexBufferSize = (uint32_t) (sizeof(SDebugVertexLine) * m_DebugVertexLinesAddList.size());

		if (neededVertexBufferSize > currentVertexBufferSize)
		{
			vkDestroyBuffer(context->GetLogicalDevice(), m_StagingVertexBuffer,       nullptr);
			vkDestroyBuffer(context->GetLogicalDevice(), m_DebugVertexBuffer,         nullptr);
			vkFreeMemory(context->GetLogicalDevice(),    m_StagingVertexBufferMemory, nullptr);
			vkFreeMemory(context->GetLogicalDevice(),    m_DebugVertexBufferMemory,   nullptr);

			m_StagingVertexBuffer = CreateBuffer(
				context,
				m_StagingVertexBufferMemory,
				(VkDeviceSize) neededVertexBufferSize,
				VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

			m_DebugVertexBuffer = CreateBuffer(
				context, 
				m_DebugVertexBufferMemory, 
				(VkDeviceSize) neededVertexBufferSize,
				VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
		}

		m_DebugVertexLinesRenderList.clear();
		m_DebugVertexLinesRenderList = m_DebugVertexLinesAddList;

		void* data;
		vkMapMemory(context->GetLogicalDevice(), m_StagingVertexBufferMemory, 0, neededVertexBufferSize, 0, &data);
		memcpy(data, m_DebugVertexLinesRenderList.data(), (size_t)neededVertexBufferSize);
		vkUnmapMemory(context->GetLogicalDevice(), m_StagingVertexBufferMemory);

		CopyBuffer(context, m_StagingVertexBuffer, m_DebugVertexBuffer, neededVertexBufferSize);

		m_DebugVertexLinesAddList.clear();
		
	}

	void CDebugManager::Cleanup(CGraphicsContext* context)
	{
		vkDestroyBuffer(context->GetLogicalDevice(), m_StagingVertexBuffer,       nullptr);
		vkDestroyBuffer(context->GetLogicalDevice(), m_DebugVertexBuffer,         nullptr);
		vkFreeMemory(context->GetLogicalDevice(),    m_StagingVertexBufferMemory, nullptr);
		vkFreeMemory(context->GetLogicalDevice(),    m_DebugVertexBufferMemory,   nullptr);


		m_DebugVertexLinesAddList.clear();
		m_DebugVertexLinesRenderList.clear();
	}

}