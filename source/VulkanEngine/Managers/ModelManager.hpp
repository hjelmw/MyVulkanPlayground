#pragma once

#include <string>
#include <vector>

#include <glm/glm.hpp>

#include "Model.hpp"
#include "GraphicsContext.hpp"


namespace NVulkanEngine
{
	class CModelManager
	{
	public:
		CModelManager()  = default;
		~CModelManager() = default;

		void AddModel(const std::string& modelFilepath);
		void AddPosition(uint32_t index, glm::vec3 position);
		void AddRotation(uint32_t index, glm::vec3 rotation);
		void AddScaling(uint32_t index,  glm::vec3 scaling);
		void AddTexturePath(uint32_t index, const std::string& textureFilepath);
		void Cleanup(CGraphicsContext* context);

		uint32_t GetCurrentModelIndex();
		CModel*  GetModel(uint32_t index);

		const uint32_t GetNumModels();

		void AllocateModelDescriptorPool(CGraphicsContext* context);
		VkDescriptorPool GetModelDescriptorPool() { return m_DescriptorPool; };

	private:
		std::vector<CModel*> m_Models{};
		int m_CurrentModelIndex = -1;

		VkDescriptorPool m_DescriptorPool = VK_NULL_HANDLE;
	};
};