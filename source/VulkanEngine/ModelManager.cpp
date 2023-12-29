#include "ModelManager.hpp"

#include <glm/glm.hpp>
#include <vector>

namespace NVulkanEngine
{
	CModelManager* CModelManager::s_ModelManagerInstance = nullptr;

	CModelManager::CModelManager()
	{
	}

	CModelManager* CModelManager::GetInstance()
	{
		if (!s_ModelManagerInstance)
		{
			s_ModelManagerInstance = new CModelManager();
		}

		return s_ModelManagerInstance;
	}

	void CModelManager::SetGraphicsContext(CGraphicsContext* context)
	{
		m_Context = context;
	}

	void CModelManager::AddModel(const std::string& modelFilepath)
	{
		CModel* model = new CModel();

		model->SetModelFilepath(modelFilepath, "./assets");

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

	void CModelManager::AddTexture(const std::string& textureFilepath)
	{
		CTexture* modelTexture = new CTexture();
		modelTexture->SetGenerateMipmaps(false);
		modelTexture->CreateTexture(m_Context, textureFilepath, VK_FORMAT_R8G8B8A8_SRGB);

		m_Models[m_CurrentModelIndex]->SetModelTexture(modelTexture);
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

	std::vector<CModel*> CModelManager::GetModels()
	{
		return m_Models;
	}

	void CModelManager::AllocateModelDescriptorPool()
	{
		/* Allocate 2 sets per frame in flight per model consisting of a single uniform buffer and combined image sampler descriptor */
		AllocateDescriptorPool(m_Context, m_DescriptorPool, (uint32_t) m_Models.size() * 2, 1, 1);
	}
}
