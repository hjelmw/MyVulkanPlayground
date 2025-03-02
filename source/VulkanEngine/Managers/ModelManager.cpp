#include "ModelManager.hpp"

#include <vector>

namespace NVulkanEngine
{
	void CModelManager::AddModelFilepath(const std::string& modelFilepath)
	{
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

	const uint32_t CModelManager::GetNumModels()
	{
		return (uint32_t)m_Models.size();
	}

	void CModelManager::SetSceneBounds(glm::AABB sceneBounds)
	{
		m_SceneBounds = sceneBounds;
	}

	glm::AABB CModelManager::GetSceneBounds()
	{
		return m_SceneBounds;
	}

	void CModelManager::Cleanup(CGraphicsContext* context)
	{
		for (int i = 0; i < m_Models.size(); i++)
		{
			m_Models[i]->Cleanup(context);
		}

		m_Models.clear();
	}
}
