#pragma once

#include <string>
#include <vector>

#include <glm/glm.hpp>
#include <glm-aabb/AABB.hpp>

#include "Utils/Model.hpp"
#include "GraphicsContext.hpp"

/*
	Stores all the models so render nodes can easily access them (geometry & shadow currently)
*/

namespace NVulkanEngine
{
	class CModelManager
	{
	public:
		CModelManager()  = default;
		~CModelManager() = default;

		void AddModelFromFilepath(const std::string& modelFilepath);
		void AddPosition(const glm::vec3& position);
		void AddRotation(const glm::vec3& rotation);
		void AddScaling(const glm::vec3& scaling);
		void AddTexturePath(const std::string& textureFilepath);
		void PushModel();

		uint32_t GetCurrentModelIndex();
		const uint32_t GetNumModels();
		const uint32_t GetNumTerrainModels();
		CModel* GetModel(uint32_t index);
		CModel* GetTerrainModel(uint32_t index);

		CModel* RegisterTerrainModel(VkBuffer& terrainVertexBuffer, VkBuffer& terrainIndexBuffer, uint32_t& numTerrainVertices, uint32_t& numTerrainIndices);
		void    UnregisterTerrainModel(CGraphicsContext* context, CModel* terrainModel);

		void SetSceneBounds(glm::AABB sceneBounds);
		glm::AABB GetSceneBounds();

		void Cleanup(CGraphicsContext* context);
	private:
		std::vector<CModel*> m_Models{};
		std::vector<CModel*> m_TerrainModels{};
		int m_CurrentModelIndex = 0;
		glm::AABB m_SceneBounds = {};
	};
};