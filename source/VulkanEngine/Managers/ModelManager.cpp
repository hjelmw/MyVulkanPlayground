#include "ModelManager.hpp"

#include <algorithm>
#include <vector>

namespace NVulkanEngine
{
	void CModelManager::AddModelFromFilepath(const std::string& modelFilepath)
	{
		if (modelFilepath.empty())
			return;

		CModel* model = new CModel();

		model->SetModelFilepath("./assets/" + modelFilepath, "./assets/models");

		m_Models.push_back(model);
	}

	void CModelManager::AddPosition(const glm::vec3& position)
	{
		glm::mat4 modelMatrix = m_Models[m_CurrentModelIndex]->GetTransform();

		modelMatrix = glm::translate(modelMatrix, position);

		m_Models[m_CurrentModelIndex]->SetTransform(modelMatrix);
	}

	void CModelManager::AddRotation(const glm::vec3& rotation)
	{
		glm::mat4 modelMatrix = m_Models[m_CurrentModelIndex]->GetTransform();

		modelMatrix = glm::rotate(modelMatrix, rotation.x, glm::vec3(1.0f, 0.0f, 0.0f));
		modelMatrix = glm::rotate(modelMatrix, rotation.y, glm::vec3(0.0f, 1.0f, 0.0f));
		modelMatrix = glm::rotate(modelMatrix, rotation.z, glm::vec3(0.0f, 0.0f, 1.0f));

		m_Models[m_CurrentModelIndex]->SetTransform(modelMatrix);
	}

	void CModelManager::AddScaling(const glm::vec3& scaling)
	{
		glm::mat4 modelMatrix = m_Models[m_CurrentModelIndex]->GetTransform();

		modelMatrix = glm::scale(modelMatrix, scaling);

		m_Models[m_CurrentModelIndex]->SetTransform(modelMatrix);
	}

	void CModelManager::AddTexturePath(const std::string& textureFilepath)
	{
		if (textureFilepath.empty())
			return;

		m_Models[m_CurrentModelIndex]->SetModelTexturePath("./assets/" + textureFilepath);
	}

	void CModelManager::PushModel()
	{
		m_CurrentModelIndex++;
	}

	uint32_t CModelManager::GetCurrentModelIndex()
	{
		return m_CurrentModelIndex;
	}

	CModel* CModelManager::GetModel(uint32_t index)
	{
		if (index > m_Models.size())
			return nullptr;

		return m_Models[index];
	}

	CModel* CModelManager::GetTerrainModel(uint32_t index)
	{
		if (index > m_TerrainModels.size())
			return nullptr;

		return m_TerrainModels[index];
	}

	const uint32_t CModelManager::GetNumModels()
	{
		return (uint32_t)m_Models.size();
	}

	const uint32_t CModelManager::GetNumTerrainModels()
	{
		return (uint32_t)m_TerrainModels.size();
	}

	CModel* CModelManager::RegisterTerrainModel(VkBuffer& terrainVertexBuffer, VkBuffer& terrainIndexBuffer, uint32_t& numTerrainVertices, uint32_t& numTerrainIndices)
	{
		const bool terrainIsValid = (terrainVertexBuffer != VK_NULL_HANDLE && terrainIndexBuffer != VK_NULL_HANDLE && numTerrainVertices > 0 && numTerrainIndices > 0);
		if (!terrainIsValid)
			return nullptr;

		CModel* terrainModel = new CModel();
		terrainModel->SetModelFromVertexAndIndexBuffers(terrainVertexBuffer, terrainIndexBuffer, numTerrainVertices, numTerrainIndices);

		m_TerrainModels.push_back(terrainModel);

		return terrainModel;
	}

	void CModelManager::UnregisterTerrainModel(SRuntimeContext* context, CModel* terrainModel)
	{
		auto it = std::find(m_TerrainModels.begin(), m_TerrainModels.end(), terrainModel);
		if (it == m_TerrainModels.end())
			return;

		m_TerrainModels.erase(it);

		terrainModel->Cleanup(context);
		delete terrainModel;
	}

	void CModelManager::SetSceneBounds(glm::AABB sceneBounds)
	{
		m_SceneBounds = sceneBounds;
	}

	glm::AABB CModelManager::GetSceneBounds()
	{
		return m_SceneBounds;
	}

	void CModelManager::Cleanup(SRuntimeContext* context)
	{
		for (int i = 0; i < m_Models.size(); i++)
		{
			m_Models[i]->Cleanup(context);
		}

		m_Models.clear();
	}
}
