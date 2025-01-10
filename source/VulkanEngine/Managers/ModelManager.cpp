#include "ModelManager.hpp"

#include <vector>

namespace NVulkanEngine
{
	void CModelManager::AddModel(const std::string& modelFilepath)
	{
		CModel* model = new CModel();

		model->SetModelFilepath("./assets/" + modelFilepath, "./assets/models");

		m_Models.push_back(model);
		m_CurrentModelIndex++;
	}

	void CModelManager::AddPosition(uint32_t index, glm::vec3 position)
	{
		glm::mat4 modelMatrix = m_Models[index]->GetTransform();

		modelMatrix = glm::translate(modelMatrix, position);

		m_Models[m_CurrentModelIndex]->SetTransform(modelMatrix);
	}

	void CModelManager::AddRotation(uint32_t index, glm::vec3 rotation)
	{
		glm::mat4 modelMatrix = m_Models[index]->GetTransform();

		modelMatrix = glm::rotate(modelMatrix, rotation.x, glm::vec3(1.0f, 0.0f, 0.0f));
		modelMatrix = glm::rotate(modelMatrix, rotation.y, glm::vec3(0.0f, 1.0f, 0.0f));
		modelMatrix = glm::rotate(modelMatrix, rotation.z, glm::vec3(0.0f, 0.0f, 1.0f));

		m_Models[m_CurrentModelIndex]->SetTransform(modelMatrix);
	}

	void CModelManager::AddScaling(uint32_t index, glm::vec3 scaling)
	{
		glm::mat4 modelMatrix = m_Models[index]->GetTransform();

		modelMatrix = glm::scale(modelMatrix, scaling);

		m_Models[m_CurrentModelIndex]->SetTransform(modelMatrix);
	}

	void CModelManager::AddTexturePath(uint32_t index, const std::string& textureFilepath)
	{
		m_Models[m_CurrentModelIndex]->SetModelTexturePath("./assets/" + textureFilepath);
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

	void CModelManager::AllocateModelDescriptorPool(CGraphicsContext* context)
	{
		/* Allocate 2 sets per frame in flight per model consisting of a single uniform buffer and combined image sampler descriptor */
		AllocateDescriptorPool(context, m_DescriptorPool, (uint32_t) m_Models.size() * 2, 1, 1);
	}

	void CModelManager::Cleanup(CGraphicsContext* context)
	{
		vkDestroyDescriptorPool(context->GetLogicalDevice(), m_DescriptorPool, nullptr);

		for (int i = 0; i < m_Models.size(); i++)
		{
			m_Models[i]->Cleanup(context);
		}
	}
}
