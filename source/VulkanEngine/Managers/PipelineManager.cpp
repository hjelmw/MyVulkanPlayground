#include "PipelineManager.hpp"

namespace NVulkanEngine
{

	void CPipelineManager::RegisterPipeline(CPipeline* pipeline)
	{
		m_Pipelines.push_back(pipeline);
	}

	void CPipelineManager::CreatePipelines(CGraphicsContext* context, VkDescriptorSetLayout desrcriptorLayout)
	{
		for (uint32_t i = 0; i < m_Pipelines.size(); i++)
		{
			m_Pipelines[i]->CreatePipeline(context, desrcriptorLayout);
		}
	}
}