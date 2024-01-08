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
		static CModelManager* GetInstance();
		~CModelManager();

		void SetGraphicsContext(CGraphicsContext* graphicsContext);

		void AddModel(const std::string& modelFilepath);
		void AddPosition(uint32_t index, glm::vec3 position);
		void AddRotation(uint32_t index, glm::vec3 rotation);
		void AddScaling(uint32_t index,  glm::vec3 scaling);
		void AddTexture(const std::string& textureFilepath);

		uint32_t GetCurrentModelIndex();
		CModel*  GetModel(uint32_t index);

		const uint32_t GetNumModels();


		void AllocateModelDescriptorPool();
		VkDescriptorPool GetModelDescriptorPool() { return m_DescriptorPool; };

	private:
		CModelManager();
		static CModelManager* s_ModelManagerInstance;

		int m_CurrentModelIndex = -1;
		std::vector<CModel*> m_Models{};

		CGraphicsContext* m_Context{};

		VkDescriptorPool m_DescriptorPool = VK_NULL_HANDLE;
	};
};