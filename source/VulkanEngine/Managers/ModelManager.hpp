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

		void AddModelFilepath(const std::string& modelFilepath);
		void AddPosition(const glm::vec3& position);
		void AddRotation(const glm::vec3& rotation);
		void AddScaling(const glm::vec3& scaling);
		void AddTexturePath(const std::string& textureFilepath);
		void PushModel();

		uint32_t GetCurrentModelIndex();
		CModel*  GetModel(uint32_t index);
		const uint32_t GetNumModels();

		void SetSceneBounds(glm::AABB sceneBounds);
		glm::AABB GetSceneBounds();

		void Cleanup(CGraphicsContext* context);
	private:
		std::vector<CModel*> m_Models{};
		int m_CurrentModelIndex = 0;
		glm::AABB m_SceneBounds = {};
	};
};