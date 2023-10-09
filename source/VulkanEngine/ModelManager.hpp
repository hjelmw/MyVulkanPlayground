#include <string>
#include <vector>

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

		uint32_t GetCurrentModelIndex();
		CModel*  GetModel(uint32_t index);

		std::vector<CModel*> GetModels();

	private:
		CModelManager();
		static CModelManager* s_ModelManagerInstance;

		int m_CurrentModelIndex = -1;
		std::vector<CModel*> m_Models{};

		CGraphicsContext* m_Context{};
	};
};